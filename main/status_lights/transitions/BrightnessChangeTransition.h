#ifndef BRIGHTNESSCHANGETRANSITION_H
#define BRIGHTNESSCHANGETRANSITION_H

#include "StatusLightTransition.h"

struct BrightnessChangeTransitionConfig
{
    float initialBrightness;
    float targetBrightness;
    int duration;
};

class BrightnessChangeTransition: public StatusLightTransition
{
private:
    float initialBrightness;
    float targetBrightness;
public:
    BrightnessChangeTransition(const BrightnessChangeTransitionConfig &config);
    void updateBrightness(int deltaTime, float &brightness);
};

#endif