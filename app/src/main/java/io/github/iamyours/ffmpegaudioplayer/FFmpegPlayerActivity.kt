package io.github.iamyours.ffmpegaudioplayer

import android.os.Bundle
import android.os.Environment
import android.os.Handler
import android.os.Message
import android.support.v7.app.AppCompatActivity
import android.widget.SeekBar
import android.widget.TextView
import kotlinx.android.synthetic.main.activity_ffmpeg_player.*

class FFmpegPlayerActivity : AppCompatActivity() {
    var volumes = arrayOf("1.0", "1.0", "1.0", "1.0")
    var tempo = "1.0"
    var total = 0
    lateinit var player: FFmpegAudioPlayer
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_ffmpeg_player)

        player = FFmpegAudioPlayer()
        val path = "${Environment.getExternalStorageDirectory()}/test"
        val srcs = arrayOf(
                "$path/a.mp3",
                "$path/b.mp3",
                "$path/c.mp3",
                "$path/d.mp3"
        )
        player.init(srcs)
        initListeners()

        object : Handler() {
            override fun handleMessage(msg: Message?) {
                val text = "${player.position()}/${player.duration()}"
                tv_time.text = text
                sendEmptyMessageDelayed(1,500)
            }
        }.sendEmptyMessageDelayed(1, 500)
    }

    lateinit var tvs: Array<TextView>


    private fun initListeners() {
        tvs = arrayOf(tv_value1, tv_value2, tv_value3, tv_value4)
        val seekBars = arrayOf(sb1, sb2, sb3, sb4)
        seekBars.forEachIndexed { index, seekBar ->
            seekBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(seekBar: SeekBar, progress: Int, fromUser: Boolean) {

                }

                override fun onStartTrackingTouch(seekBar: SeekBar?) {
                }

                override fun onStopTrackingTouch(seekBar: SeekBar) {
                    val progress = seekBar.progress / 100.0f
                    volumes[index] = String.format("%.2f", progress)
                    changeVolumes()
                }

            })
        }
        sb5.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {
            }

            override fun onStopTrackingTouch(seekBar: SeekBar) {
                val progress = seekBar.progress / 100.0f + 0.5
                tempo = String.format("%.2f", progress)
                tv_value5.text = "${tempo.toFloat() * 100}%"
                player.changeTempo(tempo)
            }
        })
        btn_play.setOnClickListener {
            player.play()
        }
    }

    private fun changeVolumes() {
        tvs.forEachIndexed { index, textView ->
            textView.text = "${volumes[index].toFloat() * 100}%"
        }
        player.changeVolumes(volumes)

    }
}