#ifndef LOADINGANIMATION_H
#define LOADINGANIMATION_H

#include <StatusLightAnimation.h>

class LoadingAnimation : public StatusLightAnimation
{
private:
  float offset;
  float ratio = 0.6f;

public:
  LoadingAnimation(const StatusLightAnimationConfig &config, StatusLights *statusLights);
  void render() override;
};

#endif