package io.github.iamyours.ffmpegaudioplayer

import android.content.Intent
import android.support.v7.app.AppCompatActivity
import android.os.Bundle
import android.os.Environment
import android.view.View
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : AppCompatActivity() {
    lateinit var player: FFmpegAudioPlayer
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        player = FFmpegAudioPlayer();
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

    fun toPlay(v: View) {
        val intent = Intent(this, FFmpegPlayerActivity::class.java)
        startActivity(intent)
    }


    fun openSLESPlay(v: View) {
        val path = "${Environment.getExternalStorageDirectory()}"
        openslesTest("$path/test1.mp3")
    }

    fun playMultiAudio(v: View) {
        val path = "${Environment.getExternalStorageDirectory()}/test"
        val srcs = arrayOf(
                "$path/a.mp3",
                "$path/b.mp3",
                "$path/c.mp3",
                "$path/d.mp3"
        )
        var srcs2 = arrayOf(
                "${Environment.getExternalStorageDirectory()}/test1.mp3"
        )
        var srcs3 = arrayOf(
                "$path/Timetravel.mp3"
        )
        player.init(srcs3)
        player.play()
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
