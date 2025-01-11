#include <PulseAnimation.h>

PulseAnimation::PulseAnimation(const StatusLightAnimationConfig &config, StatusLights *statusLights)
    : StatusLightAnimation(config, statusLights)
{
}

void PulseAnimation::render()
{
  statusLights->setPixels(red, green, blue, ((1 + sin(1.5 * pi + 2 * pi * progress)) / 2) * brightness);
  statusLights->show();
}