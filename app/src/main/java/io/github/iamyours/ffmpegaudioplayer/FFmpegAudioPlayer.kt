package io.github.iamyours.ffmpegaudioplayer

class FFmpegAudioPlayer {
    external fun init(paths: Array<String>)
    external fun play()
    companion object {
        init {
            System.loadLibrary("avutil-55")
            System.loadLibrary("swresample-2")
            System.loadLibrary("avcodec-57")
            System.loadLibrary("avfilter-6")
            System.loadLibrary("swscale-4")
            System.loadLibrary("avformat-57")
            System.loadLibrary("native-lib")
        }
    }
}