#include <FadeOutAnimation.h>

FadeOutAnimation::FadeOutAnimation(const StatusLightAnimationConfig &config, StatusLights &statusLights)
    : StatusLightAnimation(config, statusLights)
{
}

void FadeOutAnimation::render()
{
  statusLights.setPixels(red, green, blue, ((1 + sin(0.5 * pi + pi * progress)) / 2) * brightness);
  statusLights.show();
}