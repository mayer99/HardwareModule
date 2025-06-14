#ifndef STATUSLIGHTANIMATION_H
#define STATUSLIGHTANIMATION_H

#include <cstdint>
#include <algorithm>
#include <StatusLights.h>
#include <ColorChangeTransition.h>
#include <BrightnessChangeTransition.h>
#include <memory>
#include <cmath>

enum StatusLightAnimationType {
    ERROR,
    FADE_IN,
    FADE_OUT,
    LOADING,
    PULSE
};

struct StatusLightAnimationConfig
{
    StatusLightAnimationType type;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    int duration;
    float brightness;
    bool infinite;
};

class StatusLightAnimation
{
private:
    bool finished = false;
    int elapsedTime = 0;
    std::unique_ptr<ColorChangeTransition> colorChangeTransition;
    std::unique_ptr<BrightnessChangeTransition> brightnessChangeTransition;

protected:
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    int duration;
    float brightness;
    bool infinite;
    StatusLights &statusLights;
    float progress = 0.0f;
    const float pi = 3.14159f;

public:
    StatusLightAnimation(const StatusLightAnimationConfig &config, StatusLights &statusLights);
    void update(int deltaTime);
    virtual void render();
    void changeColor(uint8_t red, uint8_t green, uint8_t blue, int duration);
    void changeBrightness(float brightness, int duration);
    bool isInfinite();
    void setInfinite(bool infinite);
    int getDuration();
    bool isFinished();
};

#endif