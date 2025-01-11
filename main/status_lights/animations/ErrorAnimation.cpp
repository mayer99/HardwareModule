#include <ErrorAnimation.h>

ErrorAnimation::ErrorAnimation(const StatusLightAnimationConfig &config, StatusLights *statusLights)
    : StatusLightAnimation(config, statusLights)
{
}

void ErrorAnimation::render()
{
  if (progress > ratio)
  {
    return;
  }
  float partialProgress = progress / ratio;
  float pixelBrightness = ((1 + sin(1.5 * pi + 4 * pi * partialProgress)) / 2) * brightness;
  statusLights->setPixels(red, green, blue, pixelBrightness);
  statusLights->show();
}