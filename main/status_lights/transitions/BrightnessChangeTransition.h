#ifndef BRIGHTNESSCHANGETRANSITION_H
#define BRIGHTNESSCHANGETRANSITION_H

#include "StatusLightTransition.h"

class BrightnessChangeTransition: public StatusLightTransition
{
private:
    float initialBrightness;
    float targetBrightness;
public:
    BrightnessChangeTransition(float initialBrightness, float targetBrightness, int duration);
    void updateBrightness(int deltaTime, float &brightness);
};

#endif