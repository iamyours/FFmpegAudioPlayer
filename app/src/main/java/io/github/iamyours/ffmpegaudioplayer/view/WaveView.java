package io.github.iamyours.ffmpegaudioplayer.view;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

public class WaveView extends View {
    private short[] shortData;
    private Paint paint;
    private Path path;

    public WaveView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        paint = new Paint();
        paint.setColor(Color.WHITE);
        paint.setStyle(Paint.Style.STROKE);
    }

    public void update(short[] data) {
        shortData = data;
        invalidate();
    }

    private int index;
    private int height2;
    private int len;

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        index = 0;
        path = new Path();
        path.moveTo(0, 0);
        height2 = getHeight() / 2;
        if (shortData != null) {
            len = shortData.length;
            for (short s : shortData) {
                float y = height2 - s * height2 / 32768f;
                float x = (index++) * getWidth() / len;
                path.lineTo(x, y);
            }
        }
        canvas.drawPath(path, paint);
    }
}
