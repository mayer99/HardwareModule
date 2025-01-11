#ifndef STATUSLIGHTCONTROLLER_H
#define STATUSLIGHTCONTROLLER_H

#include "StatusLights.h"
#include <memory>
#include <chrono>
#include "StatusLightAnimation.h"
#include <semphr.h>

constexpr int FREQUENCY = 120;
constexpr int INTERVAL = 1000 / FREQUENCY;

class StatusLightHandler
{
private:
    StatusLights statusLights;
    std::unique_ptr<StatusLightAnimation> currentAnimation;
    std::unique_ptr<StatusLightAnimation> nextAnimation;
    static void updateTaskWrapper(void *args);
    void updateTask();
    SemaphoreHandle_t mutex;

public:
    StatusLightHandler();
    void scheduleAnimation();
};

#endif