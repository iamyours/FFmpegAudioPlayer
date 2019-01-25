//
// Created by yanxx on 2019/1/21.
//

#ifndef FFMPEGAUDIOPLAYER_AUDIOPLAYER_H
#define FFMPEGAUDIOPLAYER_AUDIOPLAYER_H

#include <vector>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>
#include <jni.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <pthread.h>
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"FFmpegAudioPlayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"FFmpegAudioPlayer",FORMAT,##__VA_ARGS__);

class AudioPlayer {
public:
    AudioPlayer(char **pathArr, int len);


//解码
    int fileCount;                  //输入音频文件数量
    AVFormatContext **fmt_ctx_arr;  //FFmpeg上下文数组
    AVCodecContext **codec_ctx_arr; //解码器上下文数组
    int *stream_index_arr;          //音频流索引数组

//过滤
    AVFilterGraph *graph;
    AVFilterContext **srcs;         //输入filter
    AVFilterContext *sink;          //输出filter
    char **volumes;                 //各个音频的音量
    char *tempo;                    //播放速度0.5~2.0
    int change = 0;

//AVFrame队列
    std::vector<AVFrame *> queue;   //队列，用于保存解码过滤后的AVFrame

//输入输出格式
    SwrContext *swr_ctx;            //重采样，用于将AVFrame转成pcm数据
    uint64_t in_ch_layout;
    int in_sample_rate;            //采样率
    int in_ch_layout_nb;           //输入声道数，配合swr_ctx使用
    enum AVSampleFormat in_sample_fmt; //输入音频采样格式

    uint64_t out_ch_layout;
    int out_sample_rate;            //采样率
    int out_ch_layout_nb;           //输出声道数，配合swr_ctx使用
    int max_audio_frame_size;       //最大缓冲数据大小
    enum AVSampleFormat out_sample_fmt; //输出音频采样格式

// 进度相关
    AVRational time_base;           //刻度，用于计算进度
    double total_time;              //总时长（秒)
    double current_time = 0;          //当前进度
    int isPlay = 0;                 //播放状态1：播放中

//多线程
    pthread_t decodeId;             //解码线程id
    pthread_t playId;               //播放线程id
    pthread_mutex_t mutex;          //同步锁
    pthread_cond_t not_full;        //不为满条件，生产AVFrame时使用
    pthread_cond_t not_empty;       //不为空条件，消费AVFrame时使用

//Open SL ES
    SLObjectItf engineObject;       //引擎对象
    SLEngineItf engineItf;          //引擎接口
    SLObjectItf mixObject;          //输出混音对象
    SLObjectItf playerObject;       //播放器对象
    SLPlayItf playItf;              //播放器接口
    SLAndroidSimpleBufferQueueItf bufferQueueItf;   //缓冲接口

    void play();

    void pause();

    void setPlaying();

    void decodeAudio();                     //解码音频

    int createPlayer();                     //创建播放器
    int initCodecs(char **pathArr);         //初始化解码器
    int initSwrContext();                   //初始化SwrContext
    int initFilters();                      //初始化过滤器


    AVFrame *get();

    int put(AVFrame *frame);

    void seek(double secs);

    void release();
};

}

#endif //FFMPEGAUDIOPLAYER_AUDIOPLAYER_H
