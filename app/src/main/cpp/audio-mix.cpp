//
// Created by yanxx on 2019/1/17.
//
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

//初始化amix filter
int init_amix_filter(AVFilterGraph **pGraph, AVFilterContext **srcs, AVFilterContext **pOut,
                     jsize len) {
    AVFilterGraph *graph = avfilter_graph_alloc();
    for (int i = 0; i < len; i++) {
        AVFilter *filter = avfilter_get_by_name("abuffer");
        char name[50];
        snprintf(name, sizeof(name), "src%d", i);
        AVFilterContext *abuffer_ctx = avfilter_graph_alloc_filter(graph, filter, name);
        if (avfilter_init_str(abuffer_ctx,
                              "sample_rate=48000:sample_fmt=s16p:channel_layout=stereo") < 0) {
            LOGE("error init abuffer filter");
            return -1;
        }
        srcs[i] = abuffer_ctx;
    }
    AVFilter *amix = avfilter_get_by_name("amix");
    AVFilterContext *amix_ctx = avfilter_graph_alloc_filter(graph, amix, "amix");
    char args[128];
    snprintf(args, sizeof(args), "inputs=%d:duration=first:dropout_transition=3", len);
    if (avfilter_init_str(amix_ctx, args) < 0) {
        LOGE("error init amix filter");
        return -1;
    }
    AVFilter *aformat = avfilter_get_by_name("aformat");
    AVFilterContext *aformat_ctx = avfilter_graph_alloc_filter(graph, aformat, "aformat");
    if (avfilter_init_str(aformat_ctx,
                          "sample_rates=48000:sample_fmts=s16p:channel_layouts=stereo") < 0) {
        LOGE("error init aformat filter");
        return -1;
    }
    AVFilter *sink = avfilter_get_by_name("abuffersink");
    AVFilterContext *sink_ctx = avfilter_graph_alloc_filter(graph, sink, "sink");
    avfilter_init_str(sink_ctx, NULL);
    for (int i = 0; i < len; i++) {
        if (avfilter_link(srcs[i], 0, amix_ctx, i) < 0) {
            LOGE("error link to amix");
            return -1;
        }
    }
    if (avfilter_link(amix_ctx, 0, aformat_ctx, 0) < 0) {
        LOGE("error link to aformat");
        return -1;
    }
    if (avfilter_link(aformat_ctx, 0, sink_ctx, 0) < 0) {
        LOGE("error link to sink");
        return -1;
    }
    if (avfilter_graph_config(graph, NULL) < 0) {
        LOGE("error config graph");
        return -1;
    }
    *pGraph = graph;
    *pOut = sink_ctx;
    return 0;
}

extern "C" JNIEXPORT void
JNICALL
Java_io_github_iamyours_ffmpegaudioplayer_MainActivity_mixAudio(
        JNIEnv *env,
        jobject /* this */, jobjectArray _srcs, jstring _out) {
    //将java传入的字符串数组转为c字符串数组
    jsize len = env->GetArrayLength(_srcs);
    const char *out_path = env->GetStringUTFChars(_out, 0);
    char **pathArr = (char **) malloc(len * sizeof(char *));
    int i = 0;
    for (i = 0; i < len; i++) {
        jstring str = static_cast<jstring>(env->GetObjectArrayElement(_srcs, i));
        pathArr[i] = const_cast<char *>(env->GetStringUTFChars(str, 0));
    }
    //初始化解码器数组
    av_register_all();
    AVFormatContext **fmt_ctx_arr = (AVFormatContext **) malloc(len * sizeof(AVFormatContext *));
    AVCodecContext **codec_ctx_arr = (AVCodecContext **) malloc(len * sizeof(AVCodecContext *));
    int stream_index_arr[len];
    for (int n = 0; n < len; n++) {
        AVFormatContext *fmt_ctx = avformat_alloc_context();
        fmt_ctx_arr[n] = fmt_ctx;
        const char *path = pathArr[n];

        if (avformat_open_input(&fmt_ctx, path, NULL, NULL) < 0) {//打开文件
            LOGE("could not open file:%s", path);
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
        if (audio_stream_index < 0) {
            LOGE("error find stream index");
            break;
        }
        stream_index_arr[n] = audio_stream_index;
        //获取解码器
        AVCodecContext *codec_ctx = avcodec_alloc_context3(NULL);
        codec_ctx_arr[n] = codec_ctx;
        avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[audio_stream_index]->codecpar);
        AVCodec *codec = avcodec_find_decoder(codec_ctx->codec_id);
        //打开解码器
        if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
            LOGE("could not open codec");
            return;
        }
    }
    //初始化SwrContext
    SwrContext *swr_ctx = swr_alloc();
    enum AVSampleFormat in_sample_fmt = codec_ctx_arr[0]->sample_fmt;
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int in_sample_rate = codec_ctx_arr[0]->sample_rate;
    int out_sample_rate = in_sample_rate;
    uint64_t in_ch_layout = codec_ctx_arr[0]->channel_layout;
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    swr_alloc_set_opts(swr_ctx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout,
                       in_sample_fmt,
                       in_sample_rate, 0, NULL);
    swr_init(swr_ctx);
    int out_ch_layout_nb = av_get_channel_layout_nb_channels(out_ch_layout);//声道个数
    uint8_t *out_buffer = (uint8_t *) av_malloc(MAX_AUDIO_SIZE);//重采样数据
    //初始化amix filter
    AVFilterGraph *graph;
    AVFilterContext **srcs = (AVFilterContext **) malloc(len * sizeof(AVFilterContext *));
    AVFilterContext *sink;
    avfilter_register_all();
    init_amix_filter(&graph, srcs, &sink, len);
    //开始解码
    FILE *out_file = fopen(out_path, "wb");
    AVFrame *frame = av_frame_alloc();
    AVPacket *packet = av_packet_alloc();
    int ret = 0, got_frame;
    int index = 0;
    while (1) {
        for (int i = 0; i < len; i++) {
            ret = av_read_frame(fmt_ctx_arr[i], packet);
            if (ret < 0)break;
            if (packet->stream_index == stream_index_arr[i]) {
                ret = avcodec_decode_audio4(codec_ctx_arr[i], frame, &got_frame, packet);
                if (ret < 0)break;
                if (got_frame > 0) {
                    ret = av_buffersrc_add_frame(srcs[i], frame);
                    if (ret < 0) {
                        LOGE("error add frame:%d", index);
                        break;
                    }
                }
            }
        }
        while (av_buffersink_get_frame(sink, frame) >= 0) {
            swr_convert(swr_ctx, &out_buffer, MAX_AUDIO_SIZE, (const uint8_t **) frame->data,
                        frame->nb_samples);
            int out_size = av_samples_get_buffer_size(NULL, out_ch_layout_nb, frame->nb_samples,
                                                      out_sample_fmt, 0);
            fwrite(out_buffer, 1, out_size, out_file);
        }
        if (ret < 0) {
            LOGI("error frame:%d", index);
            break;
        }
        LOGI("decode frame :%d", index);
        index++;
    }
    LOGI("finish");
}

