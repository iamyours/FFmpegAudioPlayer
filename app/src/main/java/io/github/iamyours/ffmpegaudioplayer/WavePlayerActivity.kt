package io.github.iamyours.ffmpegaudioplayer

import android.os.Bundle
import android.os.Environment
import android.support.v7.app.AppCompatActivity
import kotlinx.android.synthetic.main.activity_wave_player.*


class WavePlayerActivity : AppCompatActivity() {
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
        val t = System.currentTimeMillis()
        list.addAll(it.toMutableList())
        if (t - lastTime > 100) {
            waveView.post {
                waveView.update(ByteUtil.byteArray2ShortArray(list))
                list.clear()
            }
            lastTime = t
        }

    }


    external fun init(src: String, callback: WaveDataCallback)
}