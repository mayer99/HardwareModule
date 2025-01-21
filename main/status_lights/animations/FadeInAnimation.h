#ifndef FADEINANIMATION_H
#define FADEINANIMATION_H

#include <StatusLightAnimation.h>

class FadeInAnimation : public StatusLightAnimation
{
public:
  FadeInAnimation(const StatusLightAnimationConfig &config, StatusLights &statusLights);
  void render() override;
};

#endif