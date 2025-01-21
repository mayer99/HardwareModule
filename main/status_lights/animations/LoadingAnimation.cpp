#include <LoadingAnimation.h>

LoadingAnimation::LoadingAnimation(const StatusLightAnimationConfig& config, StatusLights &statusLights)
    : StatusLightAnimation(config, statusLights)
{
  offset = (1.0 - ratio) / (statusLights.getPixelCount() - 1);
}

void LoadingAnimation::render()
{
  for (int pixel = 0; pixel < statusLights.getPixelCount(); pixel++)
  {
    float pixelStart = pixel * offset;
    float pixelEnd = pixelStart + ratio;
    float pixelBrightness = 0.0;
    if (progress > pixelStart && progress < pixelEnd)
    {
      float pixelProgress = (progress - pixelStart) / ratio;
      pixelBrightness = ((1 + sin(1.5 * pi + 2 * pi * pixelProgress)) / 2) * brightness;
    }

    statusLights.setPixel(pixel, red, green, blue, pixelBrightness);
  }
  statusLights.show();
}