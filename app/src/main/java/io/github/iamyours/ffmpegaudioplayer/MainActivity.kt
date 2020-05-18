package io.github.iamyours.ffmpegaudioplayer

import android.content.Intent
import android.os.Bundle
import android.os.Environment
import android.support.v7.app.AppCompatActivity
import android.util.Log
import android.view.View
import com.yanzhenjie.permission.AndPermission
import com.yanzhenjie.permission.runtime.Permission
import java.io.File

class MainActivity : AppCompatActivity() {
    lateinit var player: FFmpegAudioPlayer
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        player = FFmpegAudioPlayer()
        AndPermission.with(this)
                .runtime()
                .permission(Permission.Group.STORAGE)
                .onGranted {
                    copyFiles()
                }.start()
    }

    private fun copyFiles() {
        val srcs = arrayOf("a.mp3", "b.mp3", "c.mp3", "d.mp3", "test1.mp3", "TimeTravel.mp3")
        val outDir = "${Environment.getExternalStorageDirectory()}/test/"
        val file = File(outDir)
        if (!file.exists()) file.mkdirs()
        srcs.forEach {
            val out = "$outDir$it"
            if (!File(out).exists()) {
                FileUtil.copyFromAssets(this, it, out)
            }
        }
    }

    fun decodeAudio(v: View) {
        val src = "${Environment.getExternalStorageDirectory()}/test/test1.mp3"
        val out = "${Environment.getExternalStorageDirectory()}/test/out2.pcm"
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

    fun encodeAudio(v: View) {
        val src = "${Environment.getExternalStorageDirectory()}/test/mix.pcm"
        val out = "${Environment.getExternalStorageDirectory()}/test/mix.mp3"
        encodeAudio(src, out)
    }

    fun toPlay(v: View) {
        val intent = Intent(this, FFmpegPlayerActivity::class.java)
        startActivity(intent)
    }


    fun openSLESPlay(v: View) {
        val path = "${Environment.getExternalStorageDirectory()}/test"
        val src = "$path/TimeTravel.mp3"
        val exist = File(src).exists()
        Log.e("Main", "exist:$exist")
        openslesTest(src)
    }

    fun playMultiAudio(v: View) {
        val path = "${Environment.getExternalStorageDirectory()}/test"
        val srcs = arrayOf(
                "$path/a.mp3",
                "$path/b.mp3",
                "$path/c.mp3",
                "$path/d.mp3"
        )

        player.init(srcs)
        player.play()
    }

    fun showWave(v: View) {
        startActivity(Intent(this, WavePlayerActivity::class.java))
    }

    external fun mixAudio(srcs: Array<String>, out: String)
    external fun decodeAudio(src: String, out: String)
    external fun openslesTest(src: String)
    external fun encodeAudio(str: String, out: String)

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
