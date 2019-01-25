//
// Created by yanxx on 2019/1/21.
//

#include "AudioPlayer.h"
#include <unistd.h>

void _playCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    AudioPlayer *player = (AudioPlayer *) context;
    AVFrame *frame = player->get();
    if (frame) {
        int size = av_samples_get_buffer_size(NULL, player->out_ch_layout_nb, frame->nb_samples,
                                              player->out_sample_fmt, 1);
        if (size > 0) {
            uint8_t *outBuffer = (uint8_t *) av_malloc(player->max_audio_frame_size);
            swr_convert(player->swr_ctx, &outBuffer, player->max_audio_frame_size,
                        (const uint8_t **) frame->data, frame->nb_samples);
            (*bq)->Enqueue(bq, outBuffer, size);
        }
    }
}

AudioPlayer::AudioPlayer(char **pathArr, int len) {
    //初始化
    fileCount = len;
    //默认音量1.0 速度1.0
    volumes = (char **) malloc(fileCount * sizeof(char *));
    for (int i = 0; i < fileCount; i++) {
        volumes[i] = "1.0";
    }
    tempo = "1.0";

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&not_full, NULL);
    pthread_cond_init(&not_empty, NULL);

    initCodecs(pathArr);
    avfilter_register_all();
    initSwrContext();
    initFilters();
    createPlayer();
}

/**
 * 将AVFrame加入到队列，队列长度为5时，阻塞等待
 * @param frame
 * @return
 */
int AudioPlayer::put(AVFrame *frame) {
    AVFrame *out = av_frame_alloc();
    if (av_frame_ref(out, frame) < 0)return -1;
    pthread_mutex_lock(&mutex);
    if (queue.size() == 5) {
        LOGI("queue is full,wait for put frame:%d", queue.size());
        pthread_cond_wait(&not_full, &mutex);
    }
    queue.push_back(out);
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);
    return 1;
}

/**
 * 取出AVFrame，队列为空时，阻塞等待
 * @return
 */
AVFrame *AudioPlayer::get() {
    AVFrame *out = av_frame_alloc();
    pthread_mutex_lock(&mutex);
    while (isPlay) {
        if (queue.empty()) {
            pthread_cond_wait(&not_empty, &mutex);
        } else {
            AVFrame *src = queue.front();
            if (av_frame_ref(out, src) < 0)return NULL;
            queue.erase(queue.begin());//删除取出的元素
            av_free(src);
            if (queue.size() < 5)pthread_cond_signal(&not_full);
            pthread_mutex_unlock(&mutex);
            current_time = av_q2d(time_base) * out->pts;
            LOGI("get frame:%d,time:%lf,change:%d", queue.size(), current_time, change);
            return out;
        }
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}


void AudioPlayer::decodeAudio() {
    LOGI("start decode...");
    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();
    int ret, got_frame;
    int index = 0;
    while (isPlay) {
        LOGI("decode frame:%d", index);
        if (change) {
            initFilters();
        }
        for (int i = 0; i < fileCount; i++) {
            AVFormatContext *fmt_ctx = fmt_ctx_arr[i];
            ret = av_read_frame(fmt_ctx, packet);
            if (packet->stream_index != stream_index_arr[i])continue;
            if (ret < 0) {
                LOGE("decode finish");
                goto end;
            }
            ret = avcodec_decode_audio4(codec_ctx_arr[i], frame, &got_frame, packet);
            if (ret < 0) {
                LOGE("error decode packet");
                goto end;
            }
            if (got_frame <= 0) {
                LOGE("decode error or finish");
                goto end;
            }
            ret = av_buffersrc_add_frame(srcs[i], frame);
            if (ret < 0) {
                LOGE("error add frame to filter");
                goto end;
            }
        }
        LOGI("time:%lld,%lld,%lld", frame->pkt_dts, frame->pts, packet->pts);
        while (av_buffersink_get_frame(sink, frame) >= 0) {
            frame->pts = packet->pts;
            LOGI("put frame:%d,%lld,change:%d", index, frame->pts, change);
            put(frame);
        }
        index++;
    }
    end:
    av_packet_unref(packet);
    av_frame_unref(frame);
    isPlay = 0;
}


void *_decodeAudio(void *args) {
    AudioPlayer *p = (AudioPlayer *) args;
    p->decodeAudio();
    pthread_exit(0);
}

void *_play(void *args) {
    AudioPlayer *p = (AudioPlayer *) args;
    p->setPlaying();
    pthread_exit(0);
}

void AudioPlayer::setPlaying() {
    //设置播放状态
    (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_PLAYING);
    _playCallback(bufferQueueItf, this);
}

void AudioPlayer::play() {
    LOGI("play...");
    if (isPlay) {
        (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_PLAYING);
        return;
    }
    isPlay = 1;
    seek(0);
    pthread_create(&decodeId, NULL, _decodeAudio, this);
    pthread_create(&playId, NULL, _play, this);
}

void AudioPlayer::pause() {
    (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_PAUSED);
}

int AudioPlayer::createPlayer() {
    //创建播放器
    //创建并且初始化引擎对象
//    SLObjectItf engineObject;
    slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    //获取引擎接口
//    SLEngineItf engineItf;
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineItf);
    //通过引擎接口获取输出混音
//    SLObjectItf mixObject;
    (*engineItf)->CreateOutputMix(engineItf, &mixObject, 0, 0, 0);
    (*mixObject)->Realize(mixObject, SL_BOOLEAN_FALSE);

    //设置播放器参数
    SLDataLocator_AndroidSimpleBufferQueue
            android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLuint32 samplesPerSec = (SLuint32) out_sample_rate * 1000;
    //pcm格式
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM,
                            2,//两声道
                            samplesPerSec,
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
//    SLObjectItf playerObject;
    (*engineItf)->CreateAudioPlayer(engineItf, &playerObject, &slDataSource, &audioSnk, 1, ids,
                                    req);
    (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE);

    //获取播放接口
//    SLPlayItf playItf;
    (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playItf);
    //获取缓冲接口
//    SLAndroidSimpleBufferQueueItf bufferQueueItf;
    (*playerObject)->GetInterface(playerObject, SL_IID_BUFFERQUEUE, &bufferQueueItf);

    //注册缓冲回调
    (*bufferQueueItf)->RegisterCallback(bufferQueueItf, _playCallback, this);
    return 1;
}

int AudioPlayer::initSwrContext() {
    LOGI("init swr context");
    swr_ctx = swr_alloc();
    out_sample_fmt = AV_SAMPLE_FMT_S16;
    out_ch_layout = AV_CH_LAYOUT_STEREO;
    out_ch_layout_nb = 2;
    out_sample_rate = in_sample_rate;
    max_audio_frame_size = out_sample_rate * 2;

    swr_alloc_set_opts(swr_ctx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout,
                       in_sample_fmt, in_sample_rate, 0, NULL);
    if (swr_init(swr_ctx) < 0) {
        LOGE("error init SwrContext");
        return -1;
    }
    return 1;
}

int AudioPlayer::initFilters() {
    LOGI("init filters");
    if (change)avfilter_graph_free(&graph);
    graph = avfilter_graph_alloc();

    srcs = (AVFilterContext **) malloc(fileCount * sizeof(AVFilterContext **));
    char args[128];
    AVDictionary *dic = NULL;
    //混音过滤器
    AVFilter *amix = avfilter_get_by_name("amix");
    AVFilterContext *amix_ctx = avfilter_graph_alloc_filter(graph, amix, "amix");
    snprintf(args, sizeof(args), "inputs=%d:duration=first:dropout_transition=3", fileCount);
    if (avfilter_init_str(amix_ctx, args) < 0) {
        LOGE("error init amix filter");
        return -1;
    }

    const char *sample_fmt = av_get_sample_fmt_name(in_sample_fmt);
    snprintf(args, sizeof(args), "sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
             in_sample_rate, sample_fmt, in_ch_layout);

    for (int i = 0; i < fileCount; i++) {
        AVFilter *abuffer = avfilter_get_by_name("abuffer");
        char name[50];
        snprintf(name, sizeof(name), "src%d", i);
        srcs[i] = avfilter_graph_alloc_filter(graph, abuffer, name);
        if (avfilter_init_str(srcs[i], args) < 0) {
            LOGE("error init abuffer filter");
            return -1;
        }
        //音量过滤器
        AVFilter *volume = avfilter_get_by_name("volume");
        AVFilterContext *volume_ctx = avfilter_graph_alloc_filter(graph, volume, "volume");
        av_dict_set(&dic, "volume", volumes[i], 0);
        if (avfilter_init_dict(volume_ctx, &dic) < 0) {
            LOGE("error init volume filter");
            return -1;
        }
        //将输入端链接到volume过滤器
        if (avfilter_link(srcs[i], 0, volume_ctx, 0) < 0) {
            LOGE("error link to volume filter");
            return -1;
        }
        //链接到混音amix过滤器
        if (avfilter_link(volume_ctx, 0, amix_ctx, i) < 0) {
            LOGE("error link to amix filter");
            return -1;
        }
        av_dict_free(&dic);
    }

    //变速过滤器atempo
    AVFilter *atempo = avfilter_get_by_name("atempo");
    AVFilterContext *atempo_ctx = avfilter_graph_alloc_filter(graph, atempo, "atempo");
    av_dict_set(&dic, "tempo", tempo, 0);
    if (avfilter_init_dict(atempo_ctx, &dic) < 0) {
        LOGE("error init atempo filter");
        return -1;
    }
    //输出格式
    AVFilter *aformat = avfilter_get_by_name("aformat");
    AVFilterContext *aformat_ctx = avfilter_graph_alloc_filter(graph, aformat, "aformat");
    snprintf(args, sizeof(args), "sample_rates=%d:sample_fmts=%s:channel_layouts=0x%" PRIx64,
             in_sample_rate, sample_fmt, in_ch_layout);
    if (avfilter_init_str(aformat_ctx, args) < 0) {
        LOGE("error init aformat filter");
        return -1;
    }
    //输出缓冲
    AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    sink = avfilter_graph_alloc_filter(graph, abuffersink, "sink");
    if (avfilter_init_str(sink, NULL) < 0) {
        LOGE("error init abuffersink filter");
        return -1;
    }
    //将amix链接到atempo
    if (avfilter_link(amix_ctx, 0, atempo_ctx, 0) < 0) {
        LOGE("error link to atempo filter");
        return -1;
    }
    if (avfilter_link(atempo_ctx, 0, aformat_ctx, 0) < 0) {
        LOGE("error link to aformat filter");
        return -1;
    }
    if (avfilter_link(aformat_ctx, 0, sink, 0) < 0) {
        LOGE("error link to abuffersink filter");
        return -1;
    }
    if (avfilter_graph_config(graph, NULL) < 0) {
        LOGE("error config graph");
        return -1;
    }
    LOGI("init filter success");
    change = 0;
    return 1;
}

int AudioPlayer::initCodecs(char **pathArr) {
    LOGI("init codecs");
    av_register_all();
    fmt_ctx_arr = (AVFormatContext **) malloc(fileCount * sizeof(AVFormatContext *));
    codec_ctx_arr = (AVCodecContext **) malloc(fileCount * sizeof(AVCodecContext *));
    stream_index_arr = (int *) malloc(fileCount * sizeof(int));
    for (int n = 0; n < fileCount; n++) {
        AVFormatContext *fmt_ctx = avformat_alloc_context();
        fmt_ctx_arr[n] = fmt_ctx;
        const char *path = pathArr[n];

        if (avformat_open_input(&fmt_ctx, path, NULL, NULL) < 0) {//打开文件
            LOGE("could not open file:%s", path);
            return -1;
        }
        if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {//读取音频格式文件信息
            LOGE("find stream info error");
            return -1;
        }
        //获取音频索引
        int audio_stream_index = -1;
        for (int i = 0; i < fmt_ctx->nb_streams; i++) {
            if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                audio_stream_index = i;
                LOGI("find audio stream index:%d", audio_stream_index);
                break;
            }
        }
        if (audio_stream_index < 0) {
            LOGE("error find stream index");
            return -1;
        }
        stream_index_arr[n] = audio_stream_index;
        //获取解码器
        AVCodecContext *codec_ctx = avcodec_alloc_context3(NULL);
        codec_ctx_arr[n] = codec_ctx;
        AVStream *stream = fmt_ctx->streams[audio_stream_index];
        avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[audio_stream_index]->codecpar);
        AVCodec *codec = avcodec_find_decoder(codec_ctx->codec_id);
        if (n == 0) {//输出格式和第一个相同
            in_sample_fmt = codec_ctx->sample_fmt;
            in_ch_layout = codec_ctx->channel_layout;
            in_sample_rate = codec_ctx->sample_rate;
            in_ch_layout_nb = av_get_channel_layout_nb_channels(in_ch_layout);
            max_audio_frame_size = in_sample_rate * in_ch_layout_nb;
            time_base = fmt_ctx->streams[audio_stream_index]->time_base;
            int64_t duration = stream->duration;
            total_time = av_q2d(stream->time_base) * duration;
            LOGI("total time:%lf", total_time);
        } else {//如果是多个文件，判断格式是否一致（采用率，格式、声道数）
            if (in_ch_layout != codec_ctx->channel_layout
                || in_sample_fmt != codec_ctx->sample_fmt
                || in_sample_rate != codec_ctx->sample_rate) {
                LOGE("输入文件格式相同");
                return -1;
            }
        }
        //打开解码器
        if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
            LOGE("could not open codec");
            return -1;
        }
    }
    return 1;
}

void AudioPlayer::seek(double secs) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < fileCount; i++) {
        av_seek_frame(fmt_ctx_arr[i], stream_index_arr[i], (int64_t) (secs / av_q2d(time_base)),
                      AVSEEK_FLAG_ANY);
    }
    current_time = secs;
    queue.clear();
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&mutex);
}

void AudioPlayer::release() {
    pthread_mutex_lock(&mutex);
    isPlay = 0;
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&mutex);
    if (playItf)(*playItf)->SetPlayState(playItf, SL_PLAYSTATE_STOPPED);
    if (playerObject) {
        (*playerObject)->Destroy(playerObject);
        playerObject = 0;
        bufferQueueItf = 0;
    }
    if (mixObject) {
        (*mixObject)->Destroy(mixObject);
        mixObject = 0;
    }
    if (engineObject) {
        (*engineObject)->Destroy(engineObject);
        engineItf = 0;
    }
    if (swr_ctx) {
        swr_free(&swr_ctx);
    }
    if (graph) {
        avfilter_graph_free(&graph);
    }
    for (int i = 0; i < fileCount; i++) {
        avcodec_close(codec_ctx_arr[i]);
        avformat_close_input(&fmt_ctx_arr[i]);
    }
    free(codec_ctx_arr);
    free(fmt_ctx_arr);
    LOGI("release...");
}

