#include <jni.h>
#include <android/log.h>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"FFmpegAudioPlayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"FFmpegAudioPlayer",FORMAT,##__VA_ARGS__);
#define MAX_AUDIO_SIZE 48000*4

int init_volume_filter(AVFilterGraph **pGraph, AVFilterContext **src, AVFilterContext **out,
                       char *value) {

    //初始化AVFilterGraph
    AVFilterGraph *graph = avfilter_graph_alloc();
    //获取abuffer用于接收输入端
    AVFilter *abuffer = avfilter_get_by_name("abuffer");
    AVFilterContext *abuffer_ctx = avfilter_graph_alloc_filter(graph, abuffer, "src");
    //设置参数，这里需要匹配原始音频采样率、数据格式（位数）
    if (avfilter_init_str(abuffer_ctx, "sample_rate=48000:sample_fmt=s16p:channel_layout=stereo") <
        0) {
        LOGE("error init abuffer filter");
        return -1;
    }
    //初始化volume filter
    AVFilter *volume = avfilter_get_by_name("atempo");
    AVFilterContext *volume_ctx = avfilter_graph_alloc_filter(graph, volume, "atempo");
    //这里采用av_dict_set设置参数
    AVDictionary *args = NULL;
    av_dict_set(&args, "tempo", value, 0);//调节音量为原先的一半
    if (avfilter_init_dict(volume_ctx, &args) < 0) {
        LOGE("error init volume filter");
        return -1;
    }

    AVFilter *aformat = avfilter_get_by_name("aformat");
    AVFilterContext *aformat_ctx = avfilter_graph_alloc_filter(graph, aformat, "aformat");
    if (avfilter_init_str(aformat_ctx,
                          "sample_rates=48000:sample_fmts=s16p:channel_layouts=stereo") < 0) {
        LOGE("error init aformat filter");
        return -1;
    }
    //初始化sink用于输出
    AVFilter *sink = avfilter_get_by_name("abuffersink");
    AVFilterContext *sink_ctx = avfilter_graph_alloc_filter(graph, sink, "sink");
    if (avfilter_init_str(sink_ctx, NULL) < 0) {//无需参数
        LOGE("error init sink filter");
        return -1;
    }
    //链接各个filter上下文
    if (avfilter_link(abuffer_ctx, 0, volume_ctx, 0) != 0) {
        LOGE("error link to volume filter");
        return -1;
    }
    if (avfilter_link(volume_ctx, 0, aformat_ctx, 0) != 0) {
        LOGE("error link to aformat filter");
        return -1;
    }
    if (avfilter_link(aformat_ctx, 0, sink_ctx, 0) != 0) {
        LOGE("error link to sink filter");
        return -1;
    }
    if (avfilter_graph_config(graph, NULL) < 0) {
        LOGI("error config filter graph");
        return -1;
    }
    *pGraph = graph;
    *src = abuffer_ctx;
    *out = sink_ctx;
    LOGI("init filter success...");
    return 0;
}

extern "C" JNIEXPORT void
JNICALL
Java_io_github_iamyours_ffmpegaudioplayer_MainActivity_decodeAudio(
        JNIEnv *env,
        jobject /* this */, jstring _src, jstring _out) {
    const char *src = env->GetStringUTFChars(_src, 0);
    const char *out = env->GetStringUTFChars(_out, 0);

    av_register_all();//注册所有容器解码器
    AVFormatContext *fmt_ctx = avformat_alloc_context();

    if (avformat_open_input(&fmt_ctx, "/test1.mp3", NULL, NULL) < 0) {//打开文件
        LOGE("open file error:%s",src);
        return;
    }
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {//读取音频格式文件信息
        LOGE("find stream info error");
        return;
    }
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
        return;
    }
    //分配AVPacket和AVFrame内存，用于接收音频数据，解码数据
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    int got_frame;//接收解码结果
    int index = 0;
    //pcm输出文件
    FILE *out_file = fopen(out, "wb");
    AVFilterGraph *graph;
    AVFilterContext *in_ctx;
    AVFilterContext *out_ctx;
    //注册所有过滤器
    avfilter_register_all();
    init_volume_filter(&graph, &in_ctx, &out_ctx, "0.5");
    //初始化SwrContext
    SwrContext *swr_ctx = swr_alloc();
    enum AVSampleFormat in_sample_fmt = codec_ctx->sample_fmt;
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int in_sample_rate = codec_ctx->sample_rate;
    int out_sample_rate = in_sample_rate;
    uint64_t in_ch_layout = codec_ctx->channel_layout;
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    swr_alloc_set_opts(swr_ctx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout,
                       in_sample_fmt,
                       in_sample_rate, 0, NULL);
    swr_init(swr_ctx);
    int out_ch_layout_nb = av_get_channel_layout_nb_channels(out_ch_layout);//声道个数
    uint8_t *out_buffer = (uint8_t *) av_malloc(MAX_AUDIO_SIZE);//重采样数据
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
                if (index == 1000) {//模拟动态修改音量
                    init_volume_filter(&graph, &in_ctx, &out_ctx, "1.0");
                }
                if (index == 2000) {
                    init_volume_filter(&graph, &in_ctx, &out_ctx, "0.8");
                }
                if (index == 3000) {
                    init_volume_filter(&graph, &in_ctx, &out_ctx, "1.5");
                }
                if (index == 4000) {
                    init_volume_filter(&graph, &in_ctx, &out_ctx, "2.0");
                }
                if (av_buffersrc_add_frame(in_ctx, frame) < 0) {//将frame放入输入filter上下文
                    LOGE("error add frame");
                    break;
                }
                while (av_buffersink_get_frame(out_ctx, frame) >= 0) {//从输出filter上下文中获取frame
//                    fwrite(frame->data[0], 1, static_cast<size_t>(frame->linesize[0]),
//                           out_file); //想将单个声道pcm数据写入文件
                    swr_convert(swr_ctx, &out_buffer, MAX_AUDIO_SIZE,
                                (const uint8_t **) frame->data, frame->nb_samples);
                    int out_size = av_samples_get_buffer_size(NULL, out_ch_layout_nb,
                                                              frame->nb_samples, out_sample_fmt, 0);
                    fwrite(out_buffer, 1, out_size, out_file);
                }
            }
        }
    }
    LOGI("decode finish...");
    //释放资源
    av_packet_unref(packet);
    av_frame_free(&frame);
    avcodec_close(codec_ctx);
    avformat_close_input(&fmt_ctx);
    fclose(out_file);
}