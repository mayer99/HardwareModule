#ifndef STATUSLIGHTHANDLER_H
#define STATUSLIGHTHANDLER_H

#include "StatusLights.h"
#include <memory>
#include <chrono>
#include "StatusLightAnimation.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

constexpr int FREQUENCY = 120;
constexpr int INTERVAL = 1000 / FREQUENCY;

class StatusLightHandler
{
private:
    StatusLights statusLights;
    std::unique_ptr<StatusLightAnimation> currentAnimation = nullptr;
    std::unique_ptr<StatusLightAnimation> nextAnimation = nullptr;
    SemaphoreHandle_t mutex;
    QueueHandle_t commandQueueHandle;

    void processStartAnimationCommand(const std::shared_ptr<std::vector<uint8_t>> &frame);
    void processStopAnimationCommand(const std::shared_ptr<std::vector<uint8_t>> &frame);
    void processSkipAnimationCommand(const std::shared_ptr<std::vector<uint8_t>> &frame);
    void processChangeColorCommand(const std::shared_ptr<std::vector<uint8_t>> &frame);
    void processChangeBrightnessCommand(const std::shared_ptr<std::vector<uint8_t>> &frame);
    static void updateTaskWrapper(void *args);
    void updateTask();
    static void processCommandQueueTaskWrapper(void *args);
    void processCommandQueueTask();

public:
    StatusLightHandler();
    bool queueCommand(const std::vector<uint8_t> &command);

    void startAnimation(const StatusLightAnimationConfig &config, bool interruptCurrentAnimation);
    void stopAnimation(bool interruptCurrentAnimation);
    void skipAnimation(bool interruptCurrentAnimation);
    void changeColor(uint8_t red, uint8_t green, uint8_t blue, uint16_t duration);
    void changeBrightness(float brightness, uint16_t duration);
    QueueHandle_t &getCommandQueue();
};

#endif