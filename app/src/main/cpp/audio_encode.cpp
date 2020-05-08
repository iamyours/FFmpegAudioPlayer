//
// Created by yanxx on 2020/5/3.
//
#include <jni.h>
#include <android/log.h>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"FFmpegAudioPlayer",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"FFmpegAudioPlayer",FORMAT,##__VA_ARGS__);
#define MAX_AUDIO_SIZE 48000*4

void custom_log(void *ptr, int level, const char *fmt, va_list vl) {
    LOGE(fmt, vl);
}

extern "C" JNIEXPORT void
JNICALL
Java_io_github_iamyours_ffmpegaudioplayer_MainActivity_encodeAudio(
        JNIEnv *env,
        jobject /* this */, jstring _src, jstring _out) {
    const char *src = env->GetStringUTFChars(_src, 0);
    const char *out = env->GetStringUTFChars(_out, 0);
    av_register_all();
    av_log_set_callback(custom_log);
    AVCodecContext *c = NULL;
    AVFrame *frame;
    AVPacket pkt;
    int i, j, k, ret, got_output;
    int buffer_size;
    FILE *f;
    uint16_t *samples;
    LOGI("encode start....");

    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
    if (codec == NULL) {
        LOGE("find codec error");
        return;
    }
    LOGI("find codec:%s", codec->name);
    c = avcodec_alloc_context3(codec);
    if (!c) {
        LOGE("Could not allocate audio codec context\n");
        return;
    }

    c->bit_rate = 64000;
    c->sample_fmt = AV_SAMPLE_FMT_S16P;
    c->sample_rate = 44100;
    c->codec_type = AVMEDIA_TYPE_AUDIO;
    c->channel_layout = AV_CH_LAYOUT_STEREO;
    c->channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    c->codec_id = codec->id;
    /* open it */
    int error = avcodec_open2(c, codec, NULL);
    if (error < 0) {
        LOGE("Could not open codec:%d", error);
        return;
    }

    f = fopen(out, "wb");
    if (!f) {
        LOGE("Could not open %s\n", out);
        return;
    }

    /* frame containing input raw audio */
    frame = av_frame_alloc();
    if (!frame) {
        LOGE("Could not allocate audio frame\n");
        return;
    }

    frame->nb_samples = c->frame_size;
    frame->format = c->sample_fmt;
    frame->channel_layout = c->channel_layout;

    /* the codec gives us the frame size, in samples,
    * we calculate the size of the samples buffer in bytes */
    buffer_size = av_samples_get_buffer_size(NULL, c->channels, c->frame_size,
                                             c->sample_fmt, 0);
    if (buffer_size < 0) {
        LOGE("Could not get sample buffer size\n");
        return;
    }
    samples = static_cast<uint16_t *>(av_malloc(buffer_size));
    if (!samples) {
        LOGE("Could not allocate %d bytes for samples buffer\n",
             buffer_size);
        return;
    }
    /* setup the data pointers in the AVFrame */
    ret = avcodec_fill_audio_frame(frame, c->channels, c->sample_fmt,
                                   (const uint8_t *) samples, buffer_size, 0);
    if (ret < 0) {
        LOGE("Could not setup audio frame\n");
        return;
    }

    /* encode a single tone sound */
    double_t t = 0;
    double_t tincr = M_PI * 441.0 / c->sample_rate;
    for (i = 0; i < 200; i++) {
        av_init_packet(&pkt);
        pkt.data = NULL; // packet data will be allocated by the encoder
        pkt.size = 0;

        for (j = 0; j < c->frame_size; j++) {
            samples[2 * j] = (int) (sin(t) * 10000);

            for (k = 1; k < c->channels; k++)
                samples[2 * j + k] = samples[2 * j];
            t += tincr;
        }
        /* encode the samples */
        ret = avcodec_encode_audio2(c, &pkt, frame, &got_output);
        if (ret < 0) {
            LOGE("Error encoding audio frame\n");
            return;
        }
        if (got_output) {
            fwrite(pkt.data, 1, pkt.size, f);
            av_packet_unref(&pkt);
        }
    }

    /* get the delayed frames */
    for (got_output = 1; got_output; i++) {
        ret = avcodec_encode_audio2(c, &pkt, NULL, &got_output);
        if (ret < 0) {
            LOGE("Error encoding frame\n");
            return;
        }

        if (got_output) {
            fwrite(pkt.data, 1, pkt.size, f);
            av_packet_unref(&pkt);
        }
    }
    fclose(f);

    av_freep(&samples);
    av_frame_free(&frame);
    avcodec_close(c);
    av_free(c);
    LOGI("encode finish....");

    env->ReleaseStringUTFChars(_src, src);
    env->ReleaseStringUTFChars(_out, out);
}