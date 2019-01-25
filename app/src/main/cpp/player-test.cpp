//
// Created by yanxx on 2019/1/23.
//
#include "AudioPlayer.h"

static AudioPlayer *player;

extern "C" JNIEXPORT void
JNICALL
Java_io_github_iamyours_ffmpegaudioplayer_FFmpegAudioPlayer_init(
        JNIEnv *env,
        jobject /* this */, jobjectArray _srcs) {
//将java传入的字符串数组转为c字符串数组
    jsize len = env->GetArrayLength(_srcs);
    char **pathArr = (char **) malloc(len * sizeof(char *));
    int i = 0;
    for (i = 0; i < len; i++) {
        jstring str = static_cast<jstring>(env->GetObjectArrayElement(_srcs, i));
        pathArr[i] = const_cast<char *>(env->GetStringUTFChars(str, 0));
    }
    player = new AudioPlayer(pathArr, len);
}

extern "C" JNIEXPORT void
JNICALL
Java_io_github_iamyours_ffmpegaudioplayer_FFmpegAudioPlayer_changeVolumes(
        JNIEnv *env,
        jobject /* this */, jobjectArray _volumes) {
//将java传入的字符串数组转为c字符串数组
    jsize len = env->GetArrayLength(_volumes);
    int i = 0;
    for (i = 0; i < len; i++) {
        jstring str = static_cast<jstring>(env->GetObjectArrayElement(_volumes, i));
        char *volume = const_cast<char *>(env->GetStringUTFChars(str, 0));
        player->volumes[i] = volume;
    }
    player->change = 1;
}

extern "C" JNIEXPORT void
JNICALL
Java_io_github_iamyours_ffmpegaudioplayer_FFmpegAudioPlayer_changeTempo(
        JNIEnv *env,
        jobject /* this */, jstring _tempo) {
    char *tempo = const_cast<char *>(env->GetStringUTFChars(_tempo, 0));
    player->tempo = tempo;
    player->change = 1;
}
extern "C" JNIEXPORT void
JNICALL
Java_io_github_iamyours_ffmpegaudioplayer_FFmpegAudioPlayer_play(
        JNIEnv *env,
        jobject /* this */) {
    player->play();
}

extern "C" JNIEXPORT void
JNICALL
Java_io_github_iamyours_ffmpegaudioplayer_FFmpegAudioPlayer_pause(
        JNIEnv *env,
        jobject /* this */) {
    player->pause();
}

extern "C" JNIEXPORT void
JNICALL
Java_io_github_iamyours_ffmpegaudioplayer_FFmpegAudioPlayer_release(
        JNIEnv *env,
        jobject /* this */) {
    player->release();
}
extern "C" JNIEXPORT void
JNICALL
Java_io_github_iamyours_ffmpegaudioplayer_FFmpegAudioPlayer_seek(
        JNIEnv *env,
        jobject /* this */, jdouble secs) {
    player->seek(secs);
}

extern "C" JNIEXPORT jdouble
JNICALL
Java_io_github_iamyours_ffmpegaudioplayer_FFmpegAudioPlayer_duration(
        JNIEnv *env,
        jobject /* this */) {
    return player->total_time;
}

extern "C" JNIEXPORT jdouble
JNICALL
Java_io_github_iamyours_ffmpegaudioplayer_FFmpegAudioPlayer_position(
        JNIEnv *env,
        jobject /* this */) {
    return player->current_time;
}