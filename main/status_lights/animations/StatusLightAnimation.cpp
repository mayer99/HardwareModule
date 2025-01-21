#include <StatusLightAnimation.h>

StatusLightAnimation::StatusLightAnimation(const StatusLightAnimationConfig &config, StatusLights &statusLights)
    : red(config.red), green(config.green), blue(config.blue), duration(config.duration), brightness(config.brightness), infinite(config.infinite), statusLights(statusLights)
{
}

void StatusLightAnimation::changeColor(uint8_t red, uint8_t green, uint8_t blue, int duration)
{
    if (duration == 0)
    {
        this->red = red;
        this->green = green;
        this->blue = blue;
        return;
    }
    ColorChangeTransitionConfig config = {
        .initialRed = this->red,
        .initialGreen = this->green,
        .initialBlue = this->blue,
        .targetRed = red,
        .targetGreen = green,
        .targetBlue = blue,
        .duration = duration};
    colorChangeTransition = std::make_unique<ColorChangeTransition>(config);
}

void StatusLightAnimation::changeBrightness(float brightness, int duration)
{
    if (duration == 0)
    {
        this->brightness = brightness;
        return;
    }
    BrightnessChangeTransitionConfig config = {
        .initialBrightness = this->brightness,
        .targetBrightness = brightness,
        .duration = duration};
    brightnessChangeTransition = std::make_unique<BrightnessChangeTransition>(config);
}

void StatusLightAnimation::update(int deltaTime)
{
    elapsedTime += deltaTime;
    if (elapsedTime >= duration)
    {
        progress = 1.0f;
        if (infinite)
        {
            elapsedTime = 0;
        }
        else
        {
            finished = true;
        }
    }
    else
    {
        progress = float(elapsedTime) / duration;
    }

    if (colorChangeTransition != nullptr)
    {
        colorChangeTransition->updateColor(deltaTime, red, green, blue);
        if (colorChangeTransition->isFinished())
        {
            colorChangeTransition = nullptr;
        }
    }

    if (brightnessChangeTransition != nullptr)
    {
        brightnessChangeTransition->updateBrightness(deltaTime, brightness);
        if (brightnessChangeTransition->isFinished())
        {
            brightnessChangeTransition = nullptr;
        }
    }
}

void StatusLightAnimation::render()
{
}

bool StatusLightAnimation::isInfinite()
{
    return infinite;
}

void StatusLightAnimation::setInfinite(bool infinite)
{
    this->infinite = infinite;
}

int StatusLightAnimation::getDuration()
{
    return duration;
}

bool StatusLightAnimation::isFinished()
{
    return finished;
}