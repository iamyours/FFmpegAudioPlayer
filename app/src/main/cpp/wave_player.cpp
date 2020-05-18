//
// Created by yanxx on 2019/1/21.
//
#include <jni.h>
#include <android/log.h>
#include <string>
#include <SLES/OpenSLES_Android.h>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <pthread.h>
}
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"FFmpegAudioPlayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"FFmpegAudioPlayer",FORMAT,##__VA_ARGS__);
static pthread_mutex_t mutex;
//条件变量
static pthread_cond_t notfull; //队列未达到最大缓冲容量，存
static pthread_cond_t notempty;//队列不为空，取
static std::vector<AVFrame *> queue;
static SwrContext *swr_ctx;
static int out_ch_layout_nb;
static enum AVSampleFormat out_sample_fmt;
static JNIEnv *jniEnv;
static jobject jCallbackObj;
static jmethodID methodId;
static JavaVM *jvm;
static char *filePath;

#define QUEUE_SIZE 5
#define MAX_AUDIO_SIZE 44100*2

/**
 * 调用java层代码
 * @param frame
 */
void callMethod(AVFrame *frame) {
    LOGI("call method");
    uint8_t *data = (uint8_t *) av_malloc(MAX_AUDIO_SIZE);
    swr_convert(swr_ctx, &data, MAX_AUDIO_SIZE, (const uint8_t **) frame->data,
                frame->nb_samples);
    int out_size = av_samples_get_buffer_size(NULL, out_ch_layout_nb, frame->nb_samples,
                                              out_sample_fmt, 0);
    jbyteArray arr = jniEnv->NewByteArray(out_size);
    jbyte *bytes = reinterpret_cast<jbyte *>(data);
    jniEnv->SetByteArrayRegion(arr, 0, out_size, bytes);
    jniEnv->CallVoidMethod(jCallbackObj, methodId, arr);
    jniEnv->ReleaseByteArrayElements(arr, bytes, JNI_FALSE);
    jniEnv->DeleteLocalRef(arr);
}

void addFrame2(AVFrame *src) {
    AVFrame *frame = av_frame_alloc();
    if (av_frame_ref(frame, src) >= 0) {//复制frame
        callMethod(frame);
        pthread_mutex_lock(&mutex);
        if (queue.size() == QUEUE_SIZE) {
            LOGI("wait for add frame...%d", queue.size());
            pthread_cond_wait(&notfull, &mutex);//等待队列不为满信号
        }
        queue.push_back(frame);
        pthread_cond_signal(&notempty);//发送不为空信号
        pthread_mutex_unlock(&mutex);
    }
}

AVFrame *getFrame2() {
    pthread_mutex_lock(&mutex);
    while (true) {
        if (!queue.empty()) {
            AVFrame *out = av_frame_alloc();
            AVFrame *src = queue.front();
            if (av_frame_ref(out, src) < 0)break;
            queue.erase(queue.begin());//删除元素
            av_free(src);
            if (queue.size() < QUEUE_SIZE)pthread_cond_signal(&notfull);//发送notfull信号
            pthread_mutex_unlock(&mutex);
            return out;
        } else {//为空等待添加
            LOGI("wait for get frame");
            pthread_cond_wait(&notempty, &mutex);
        }
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int getPCM2(uint8_t **out) {
    AVFrame *frame = getFrame2();
    if (frame) {
        uint8_t *data = (uint8_t *) av_malloc(MAX_AUDIO_SIZE);
        swr_convert(swr_ctx, &data, MAX_AUDIO_SIZE, (const uint8_t **) frame->data,
                    frame->nb_samples);
        int out_size = av_samples_get_buffer_size(NULL, out_ch_layout_nb, frame->nb_samples,
                                                  out_sample_fmt, 0);
        *out = data;
        return out_size;
    }
    return 0;
}

void playCallback2(SLAndroidSimpleBufferQueueItf bq, void *args) {
    //获取pcm数据
    uint8_t *data;
    LOGI("Enqueue...");
    SLuint32 size = (SLuint32) getPCM2(&data);
    if (size > 0) {
        (*bq)->Enqueue(bq, data, size);
    }
}


void *decodeAudio2(void *args) {
    LOGI("start decode...%s", filePath);
    LOGI("0....");
    LOGI("1....");
    jvm->AttachCurrentThread(&jniEnv, NULL);
    LOGI("2....");
    av_register_all();
    LOGI("av_register_all...")
    AVFormatContext *fmt_ctx = avformat_alloc_context();
    if (avformat_open_input(&fmt_ctx, filePath, NULL, NULL) < 0) {
        LOGE("error open file:%s", filePath);
        return NULL;
    }
    LOGI("open file:%s", filePath);
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        LOGE("error find stream info");
        return NULL;
    }
    LOGI("find stream info");
    //获取音频索引
    int audio_stream_index = -1;
    for (int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            LOGI("find audio stream index");
            break;
        }
    }
    //获取解码器
    AVCodecContext *codec_ctx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[audio_stream_index]->codecpar);
    AVCodec *codec = avcodec_find_decoder(codec_ctx->codec_id);
    //打开解码器
    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        LOGE("could not open codec");
        return NULL;
    }
    //初始化SwrContext
    swr_ctx = swr_alloc();
    enum AVSampleFormat in_sample_fmt = codec_ctx->sample_fmt;
    out_sample_fmt = AV_SAMPLE_FMT_S16;
    int in_sample_rate = codec_ctx->sample_rate;
    int out_sample_rate = in_sample_rate;
    uint64_t in_ch_layout = codec_ctx->channel_layout;
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    swr_alloc_set_opts(swr_ctx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout,
                       in_sample_fmt,
                       in_sample_rate, 0, NULL);
    swr_init(swr_ctx);
    out_ch_layout_nb = av_get_channel_layout_nb_channels(out_ch_layout);//声道个数
    uint8_t *out_buffer = (uint8_t *) av_malloc(MAX_AUDIO_SIZE);//重采样数据

    //分配AVPacket和AVFrame内存，用于接收音频数据，解码数据
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    int got_frame;//接收解码结果
    int index = 0;

    //初始化
    while (av_read_frame(fmt_ctx, packet) == 0) {//将音频数据读入packet
        if (packet->stream_index == audio_stream_index) {//取音频索引packet
            if (avcodec_decode_audio4(codec_ctx, frame, &got_frame, packet) <
                0) {//将packet解码成AVFrame
                LOGE("decode error:%d", index);
                break;
            }
            if (got_frame > 0) {
                LOGI("decode frame:%d", index++);
                addFrame2(frame);
            }
        }
    }
    LOGI("decode finish...");
    //释放资源
    av_packet_unref(packet);
    av_frame_free(&frame);
    avcodec_close(codec_ctx);
    avformat_close_input(&fmt_ctx);
}

extern "C" JNIEXPORT void
JNICALL Java_io_github_iamyours_ffmpegaudioplayer_WavePlayerActivity_init(JNIEnv *env,
                                                                          jobject /* this */,
                                                                          jstring _path,
                                                                          jobject callbackObj) {

    jclass callbackClass = env->GetObjectClass(callbackObj);

    jmethodID mid = env->GetMethodID(callbackClass, "onDataCapture", "([B)V");
    methodId = mid;
    jCallbackObj = env->NewGlobalRef(callbackObj);
//初始化同步锁和条件变量
    jniEnv = env;
    env->GetJavaVM(&jvm);
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&notfull, NULL);
    pthread_cond_init(&notempty, NULL);

//初始化解码线程
    pthread_t pid;
    filePath = (char *) env->GetStringUTFChars(_path, 0);
    LOGI("openslesTest:%s", filePath)
    pthread_create(&pid, NULL, decodeAudio2, env);

    //创建播放器
    //创建并且初始化引擎对象
    SLObjectItf engineObject;
    slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    //获取引擎接口
    SLEngineItf engineItf;
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineItf);
    //通过引擎接口获取输出混音
    SLObjectItf mixObject;
    (*engineItf)->CreateOutputMix(engineItf, &mixObject, 0, 0, 0);
    (*mixObject)->Realize(mixObject, SL_BOOLEAN_FALSE);

    //设置播放器参数
    SLDataLocator_AndroidSimpleBufferQueue
            android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    //pcm格式
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM,
                            2,//两声道
                            SL_SAMPLINGRATE_44_1,//48000采样率
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,//
                            SL_BYTEORDER_LITTLEENDIAN};

    SLDataSource slDataSource = {&android_queue, &pcm};

    //输出管道
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, mixObject};
    SLDataSink audioSnk = {&outputMix, NULL};

    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    //通过引擎接口，创建并且初始化播放器对象
    SLObjectItf playerObject;
    (*engineItf)->CreateAudioPlayer(engineItf, &playerObject, &slDataSource, &audioSnk, 1, ids,
                                    req);
    (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE);

    //获取播放接口
    SLPlayItf playItf;
    (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playItf);
    //获取缓冲接口
    SLAndroidSimpleBufferQueueItf bufferQueueItf;
    (*playerObject)->GetInterface(playerObject, SL_IID_BUFFERQUEUE, &bufferQueueItf);

    //注册缓冲回调
    (*bufferQueueItf)->RegisterCallback(bufferQueueItf, playCallback2, NULL);
    //设置播放状态
    (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_PLAYING);
    playCallback2(bufferQueueItf, NULL);
}