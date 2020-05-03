package io.github.iamyours.ffmpegaudioplayer;

import android.content.Context;

import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

public class FileUtil {
    public static void copyFromAssets(Context context, String src, String out) {
        InputStream myInput;
        OutputStream myOutput = null;
        try {
            myOutput = new FileOutputStream(out);
            myInput = context.getAssets().open(src);
            byte[] buffer = new byte[1024];
            int length = myInput.read(buffer);
            while (length > 0) {
                myOutput.write(buffer, 0, length);
                length = myInput.read(buffer);
            }

            myOutput.flush();
            myInput.close();
            myOutput.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
