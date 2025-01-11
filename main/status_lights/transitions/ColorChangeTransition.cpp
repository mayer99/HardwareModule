#include <ColorChangeTransition.h>

ColorChangeTransition::ColorChangeTransition(uint8_t initialRed, uint8_t initialGreen, uint8_t initialBlue, uint8_t targetRed, uint8_t targetGreen, uint8_t targetBlue, int duration)
    : StatusLightTransition(duration), initialRed(initialRed), initialGreen(initialGreen), initialBlue(initialBlue), targetRed(targetRed), targetGreen(targetGreen), targetBlue(targetBlue)
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
    float progress = std::clamp(float(elapsedTime) / duration, 0.0f, 1.0f);
    red = static_cast<uint8_t>(initialRed + (targetRed - initialRed) * progress);
    green = static_cast<uint8_t>(initialGreen + (targetGreen - initialGreen) * progress);
    blue = static_cast<uint8_t>(initialBlue + (targetBlue - initialBlue) * progress);
}