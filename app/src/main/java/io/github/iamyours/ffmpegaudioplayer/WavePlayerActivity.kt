package io.github.iamyours.ffmpegaudioplayer

import android.os.Bundle
import android.os.Environment
import android.support.v7.app.AppCompatActivity
import android.util.Log
import kotlinx.android.synthetic.main.activity_wave_player.*
import java.io.File
import java.io.FileOutputStream


class WavePlayerActivity : AppCompatActivity() {
    val file = File("/data/data/io.github.iamyours.ffmpegaudioplayer/out.pcm")
    val fos = FileOutputStream(file)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_wave_player)
        val path = "${Environment.getExternalStorageDirectory()}/test"
        val src = "$path/TimeTravel.mp3"
        init(src, WaveDataCallback {
            updateWave(it)
        })
    }

    private var lastTime = System.currentTimeMillis()
    private val list = ArrayList<Byte>()
    private fun updateWave(it: ByteArray) {
        Log.i("Wave", "size:${it.size}")
        val t = System.currentTimeMillis()
        list.addAll(it.toMutableList())
        if (t - lastTime > 100) {
            if (list.size < 2000) return
            waveView.post {
                waveView.update(ByteUtil.byteArray2ShortArray(list))
                list.clear()
            }
            lastTime = t
        }

    }


    external fun init(src: String, callback: WaveDataCallback)
}