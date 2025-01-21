#ifndef PULSEANIMATION_H
#define PULSEANIMATION_H

#include <StatusLightAnimation.h>

class PulseAnimation : public StatusLightAnimation
{
public:
  PulseAnimation(const StatusLightAnimationConfig &config, StatusLights &statusLights);
  void render() override;
};

#endif