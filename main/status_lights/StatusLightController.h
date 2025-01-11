#ifndef STATUSLIGHTCONTROLLER_H
#define STATUSLIGHTCONTROLLER_H

#include "StatusLights.h"
#include <memory>
#include <chrono>
#include "StatusLightAnimation.h"

constexpr int FREQUENCY = 120;
constexpr int INTERVAL = 1000 / FREQUENCY;

class StatusLightController
{
private:
    StatusLights statusLights;
    std::unique_ptr<StatusLightAnimation> currentAnimation;
    std::unique_ptr<StatusLightAnimation> nextAnimation;
    std::chrono::steady_clock::time_point lastExecutionTime = std::chrono::steady_clock::now();
    static void updateTask(void* args);

public:
    StatusLightController();
    void update();
    void showErrorAnimation();
    void showStartupAnimation();
    void stopAnimation(bool interrupt);
    void skipAnimation(bool interrupt);
    void scheduleAnimation(std::unique_ptr<StatusLightAnimation> animation, bool interrupt);
    void test();
    void schedulePulseAnimation();
};

#endif