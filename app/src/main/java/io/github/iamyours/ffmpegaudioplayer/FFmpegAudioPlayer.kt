package io.github.iamyours.ffmpegaudioplayer

class FFmpegAudioPlayer {
    /**
     * 初始化
     */
    external fun init(paths: Array<String>)

    /**
     * 播放
     */
    external fun play()

    /**
     * 暂停
     */
    external fun pause()

    /**
     * 释放资源
     */
    external fun release()

    /**
     * 修改每个音量
     */
    external fun changeVolumes(volumes: Array<String>)

    /**
     * 变速
     */
    external fun changeTempo(tempo: String)

    /**
     * 总时长 秒
     */
    external fun duration(): Double

    /**
     * 当前进度 秒
     */
    external fun position(): Double

    /**
     * 进度跳转
     */
    external fun seek(sec: Double)

    external fun callback(callback: Callback)

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