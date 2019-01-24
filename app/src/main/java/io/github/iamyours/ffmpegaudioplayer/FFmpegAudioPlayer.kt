package io.github.iamyours.ffmpegaudioplayer

class FFmpegAudioPlayer {
    external fun init(paths: Array<String>)
    external fun play()
    /**
     * 修改每个音量
     */
    external fun changeVolumes(volumes: Array<String>)

    /**
     * 变速
     */
    external fun changeTempo(tempo: String)

    external fun duration(): Double

    external fun position(): Double

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