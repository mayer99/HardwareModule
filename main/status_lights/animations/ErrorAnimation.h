#ifndef ERRORANIMATION_H
#define ERRORANIMATION_H

#include <StatusLightAnimation.h>

class ErrorAnimation : public StatusLightAnimation
{
private:
  float pi = 3.14159f;
  float ratio = 0.5f;

public:
  ErrorAnimation(const StatusLightAnimationConfig &config, StatusLights *statusLights);
  void render() override;
};

#endif