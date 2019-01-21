package io.github.iamyours.ffmpegaudioplayer

import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.os.Environment
import android.view.View
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
    }

    fun decodeAudio(v: View) {
        val src = "${Environment.getExternalStorageDirectory()}/test1.mp3"
        val out = "${Environment.getExternalStorageDirectory()}/out2.pcm"
        decodeAudio(src, out)
    }

    fun mixAudio(v: View) {
        val path = "${Environment.getExternalStorageDirectory()}/test"
        val srcs = arrayOf(
                "$path/a.mp3",
                "$path/b.mp3",
                "$path/c.mp3",
                "$path/d.mp3"
        )
        mixAudio(srcs, "$path/mix.pcm")
    }

    fun openSLESPlay(v: View) {
        val path = "${Environment.getExternalStorageDirectory()}"
        openslesTest("$path/test1.mp3")
    }

    external fun mixAudio(srcs: Array<String>, out: String)
    external fun decodeAudio(src: String, out: String)
    external fun openslesTest(src: String)

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
