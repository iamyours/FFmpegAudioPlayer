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
Java_io_github_iamyours_ffmpegaudioplayer_FFmpegAudioPlayer_play(
        JNIEnv *env,
        jobject /* this */) {
    player->play();
}