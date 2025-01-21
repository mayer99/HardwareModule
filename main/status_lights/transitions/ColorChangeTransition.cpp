#include <ColorChangeTransition.h>

ColorChangeTransition::ColorChangeTransition(const ColorChangeTransitionConfig &config)
    : StatusLightTransition(config.duration), initialRed(config.initialRed), initialGreen(config.initialGreen), initialBlue(config.initialBlue), targetRed(config.targetRed), targetGreen(config.targetGreen), targetBlue(config.targetBlue)
{
}

void ColorChangeTransition::updateColor(int deltaTime, uint8_t &red, uint8_t &green, uint8_t &blue)
{
    elapsedTime += deltaTime;
    if (elapsedTime >= duration)
    {
        finished = true;
        red = targetRed;
        green = targetGreen;
        blue = targetBlue;
        return;
    }
    float progress = float(elapsedTime) / duration;
    red = static_cast<uint8_t>(initialRed + (targetRed - initialRed) * progress);
    green = static_cast<uint8_t>(initialGreen + (targetGreen - initialGreen) * progress);
    blue = static_cast<uint8_t>(initialBlue + (targetBlue - initialBlue) * progress);
}