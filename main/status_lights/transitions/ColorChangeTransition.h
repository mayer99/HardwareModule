#ifndef COLORCHANGETRANSITION_H
#define COLORCHANGETRANSITION_H

#include "StatusLightTransition.h"
#include <cstdint>

class ColorChangeTransition: public StatusLightTransition
{
private:
    uint8_t initialRed;
    uint8_t initialGreen;
    uint8_t initialBlue;
    uint8_t targetRed;
    uint8_t targetGreen;
    uint8_t targetBlue;
public:
    ColorChangeTransition(uint8_t initialRed, uint8_t initialGreen, uint8_t initialBlue, uint8_t targetRed, uint8_t targetGreen, uint8_t targetBlue, int duration);
    void updateColor(int deltaTime, uint8_t &red, uint8_t &green, uint8_t &blue);
};

#endif