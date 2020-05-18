package io.github.iamyours.ffmpegaudioplayer.view;

import android.graphics.Canvas;
import android.graphics.Rect;

abstract public class Renderer
{
  // Have these as members, so we don't have to re-create them each time
  protected float[] mPoints;
  protected float[] mFFTPoints;
  public Renderer()
  {
  }

  // As the display of raw/FFT audio will usually look different, subclasses
  // will typically only implement one of the below methods
  /**
   * Implement this method to render the audio data onto the canvas
   * @param canvas - Canvas to draw on
   * @param data - Data to render
   * @param rect - Rect to render into
   */
  abstract public void onRender(Canvas canvas, AudioData data, Rect rect);

  /**
   * Implement this method to render the FFT audio data onto the canvas
   * @param canvas - Canvas to draw on
   * @param data - Data to render
   * @param rect - Rect to render into
   */


  // These methods should actually be called for rendering
  /**
   * Render the audio data onto the canvas
   * @param canvas - Canvas to draw on
   * @param data - Data to render
   * @param rect - Rect to render into
   */
  final public void render(Canvas canvas, AudioData data, Rect rect)
  {
    if (mPoints == null || mPoints.length < data.bytes.length * 4) {
      mPoints = new float[data.bytes.length * 4];
    }

    onRender(canvas, data, rect);
  }

}