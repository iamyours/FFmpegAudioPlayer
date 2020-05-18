package io.github.iamyours.ffmpegaudioplayer;

import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class ByteUtil {
    public static byte combine(ArrayList<Byte> bytes, int index) {
        if (bytes == null) return 0;
        int b1 = bytes.get(index);
        int b2 = bytes.get(index + 1);
        if (b1 < 0) b1 += 256;
        if (b2 < 0) b2 += 256;
        return (byte) (b1 + b2);
    }

    public static short toShort(byte[] bytes, int index) {
        return (short) ((bytes[index + 1] << 8) | (bytes[index] & 0xff));
    }

    public static int toInt(byte[] bytes, int index) {
        return ((bytes[index + 1] << 8) | (bytes[index] & 0xff));
    }

    public static byte toByte(List<Byte> list, int index) {
        return (byte) ((toInt(list, index) + toInt(list, index + 2)) / 2);
    }

    public static int toInt(List<Byte> list, int index) {
        return ((list.get(index + 1) & 0xff) << 8) | (list.get(index) & 0xff);
    }

    public static double[] toDoubleArray(List<Byte> list, int size) {
        double[] result = new double[size];
        int step = list.size() / (size * 2);
        double maxDb = 20 * Math.log10(Math.pow(2, 16) - 1);
        int index = 0;
        for (int i = 0; i < size; i++) {
            int count = 0;
            double total = 0;
            while (count++ < step) {
                int pcm = toInt(list, index);
                total += pcm;
                index += 2;
            }
            double avg = total / step;
            double db = Math.log10(avg) * 20;
            result[i] = db / maxDb;
        }
        Log.i("Main", Arrays.toString(result));
        return result;
    }

    public static short[] byteArray2ShortArray(byte[] data) {
        int items = data.length / 2;
        short[] retVal = new short[items];
        for (int i = 0; i < retVal.length; i++)
            retVal[i] = (short) ((data[i * 2] & 0xff) | (data[i * 2 + 1] & 0xff) << 8);

        return retVal;
    }

    public static short[] byteArray2ShortArray(List<Byte> data) {
        int items = data.size() / 2;
        short[] retVal = new short[items];
        for (int i = 0; i < retVal.length; i++)
            retVal[i] = (short) ((data.get(i * 2) & 0xff) | (data.get(i * 2 + 1) & 0xff) << 8);

        return retVal;
    }
}
