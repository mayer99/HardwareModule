#include <BrightnessChangeTransition.h>

BrightnessChangeTransition::BrightnessChangeTransition(float initialBrightness, float targetBrightness, int duration)
    : StatusLightTransition(duration), initialBrightness(initialBrightness), targetBrightness(targetBrightness)
{
}

void BrightnessChangeTransition::updateBrightness(int deltaTime, float &brightness)
{
    elapsedTime += deltaTime;
    if (elapsedTime >= duration)
    {
        finished = true;
        brightness = targetBrightness;
        return;
    }
    float progress = std::clamp(float(elapsedTime) / duration, 0.0f, 1.0f);
    brightness = initialBrightness + (targetBrightness - initialBrightness) * progress;
}

