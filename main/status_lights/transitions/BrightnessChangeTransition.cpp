#include <BrightnessChangeTransition.h>

BrightnessChangeTransition::BrightnessChangeTransition(const BrightnessChangeTransitionConfig &config)
    : StatusLightTransition(config.duration), initialBrightness(config.initialBrightness), targetBrightness(config.targetBrightness)
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
    float progress = float(elapsedTime) / duration;
    brightness = initialBrightness + (targetBrightness - initialBrightness) * progress;
}

