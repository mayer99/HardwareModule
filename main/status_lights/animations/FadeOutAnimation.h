#ifndef FADEOUTANIMATION_H
#define FADEOUTANIMATION_H

#include <StatusLightAnimation.h>

class FadeOutAnimation : public StatusLightAnimation
{
public:
  FadeOutAnimation(const StatusLightAnimationConfig &config, StatusLights *statusLights);
  void render() override;
};

#endif