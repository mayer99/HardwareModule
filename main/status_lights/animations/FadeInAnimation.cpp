#include <FadeInAnimation.h>

FadeInAnimation::FadeInAnimation(const StatusLightAnimationConfig &config, StatusLights *statusLights)
    : StatusLightAnimation(config, statusLights)
{
}

void FadeInAnimation::render()
{
  statusLights->setPixels(red, green, blue, ((1 + sin(1.5 * pi + pi * progress)) / 2) * brightness);
  statusLights->show();
}