#include <StatusLightHandler.h>
#include "LoadingAnimation.h"
#include "PulseAnimation.h"
#include "FadeInAnimation.h"
#include "FadeOutAnimation.h"
#include <CommandHandler.h>
#include <freertos/queue.h>
#include <esp_log.h>
#include "ErrorAnimation.h"
#include <esp_task_wdt.h>

StatusLightHandler::StatusLightHandler()
{
    statusLights.setPixels(0, 0, 0);
    statusLights.show();

    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr)
    {
        ESP_LOGE("StatusLightHandler", "Failed to create mutex");
        return;
    }

    commandQueueHandle = xQueueCreate(25, sizeof(std::shared_ptr<std::vector<uint8_t>>));
    if (commandQueueHandle == nullptr)
    {
        ESP_LOGE("StatusLightHandler", "Failed to create queue");
        return;
    }

    xTaskCreate(updateTaskWrapper, "StatusLightHandlerUpdateTask", 8128, this, 5, nullptr);
    xTaskCreate(processCommandQueueTaskWrapper, "StatusLightHandlerProcessCommandQueueTask", 8128, this, 5, nullptr);
}

void StatusLightHandler::processStartAnimationCommand(const std::shared_ptr<std::vector<uint8_t>> &frame)
{
    if (frame->size() != 17)
    {
        ESP_LOGW("StatusLightHandler", "Invalid frame size for command 0x01 (StartAnimationCommand): %d", frame->size());
        return;
    } // FF 00 11 01 01 01 00 FF 00 07 D0 7F 01 00 02 59 FE
    uint8_t animationId = (*frame)[5];
    uint8_t red = (*frame)[6];
    uint8_t green = (*frame)[7];
    uint8_t blue = (*frame)[8];
    uint16_t duration = ((*frame)[9] << 8) | (*frame)[10];
    uint8_t brightnessLevel = (*frame)[11];
    float brightness = static_cast<float>(brightnessLevel) / 255.0f;
    bool infinite = static_cast<bool>((*frame)[12]);
    bool interruptCurrentAnimation = static_cast<bool>((*frame)[13]);

    ESP_LOGI("StatusLightHandler", "Command 0x01 (StartAnimationCommand): AnimationId=%d, Red=%d, Green=%d, Blue=%d, Duration=%d, Brightness=%f, Infinite=%d, InterruptCurrentAnimation=%d",
             animationId, red, green, blue, duration, brightness, infinite, interruptCurrentAnimation);

    StatusLightAnimationType type;
    switch (animationId)
    {
    case 0x01:
        type = StatusLightAnimationType::LOADING;
        break;
    case 0x02:
        type = StatusLightAnimationType::FADE_IN;
        break;
    case 0x03:
        type = StatusLightAnimationType::FADE_OUT;
        break;
    case 0x04:
        type = StatusLightAnimationType::ERROR;
        break;
    case 0x05:
        type = StatusLightAnimationType::PULSE;
        break;
    default:
        ESP_LOGW("StatusLightHandler", "Unknown AnimationId (valid options range from 0x01 to 0x05): %d", animationId);
        return;
    }
    StatusLightAnimationConfig config = {
        .type = type,
        .red = red,
        .green = green,
        .blue = blue,
        .duration = duration,
        .brightness = brightness,
        .infinite = infinite};
    startAnimation(config, interruptCurrentAnimation);
}

void StatusLightHandler::processStopAnimationCommand(const std::shared_ptr<std::vector<uint8_t>> &frame)
{
    if (frame->size() != 9)
    {
        ESP_LOGW("StatusLightHandler", "Invalid frame size for command 0x02 (StopAnimationCommand): %d", frame->size());
        return;
    }
    bool interruptCurrentAnimation = static_cast<bool>((*frame)[5]);
    ESP_LOGI("StatusLightHandler", "Command 0x02 (StopAnimationCommand): InterruptCurrentAnimation=%d", interruptCurrentAnimation);
    stopAnimation(interruptCurrentAnimation);
}

void StatusLightHandler::processSkipAnimationCommand(const std::shared_ptr<std::vector<uint8_t>> &frame)
{
    if (frame->size() != 9)
    {
        ESP_LOGW("StatusLightHandler", "Invalid frame size for command 0x03 (SkipAnimationCommand): %d", frame->size());
        return;
    }
    bool interruptCurrentAnimation = static_cast<bool>((*frame)[5]);
    ESP_LOGI("StatusLightHandler", "Command 0x03 (SkipAnimationCommand): InterruptCurrentAnimation=%d", interruptCurrentAnimation);
    skipAnimation(interruptCurrentAnimation);
}

void StatusLightHandler::processChangeColorCommand(const std::shared_ptr<std::vector<uint8_t>> &frame)
{
    if (frame->size() != 13)
    {
        ESP_LOGW("StatusLightHandler", "Invalid frame size for command 0x04 (ChangeColorCommand): %d", frame->size());
        return;
    }
    uint8_t red = (*frame)[5];
    uint8_t green = (*frame)[6];
    uint8_t blue = (*frame)[7];
    uint16_t duration = ((*frame)[8] << 8) | (*frame)[9];

    ESP_LOGI("StatusLightHandler", "Command 0x04 (ChangeColorCommand): Red=%d, Green=%d, Blue=%d, Duration=%d", red, green, blue, duration);
    changeColor(red, green, blue, duration);
}

void StatusLightHandler::processChangeBrightnessCommand(const std::shared_ptr<std::vector<uint8_t>> &frame)
{
    if (frame->size() != 11)
    {
        ESP_LOGW("StatusLightHandler", "Invalid frame size for command 0x05 (ChangeBrightnessCommand): %d", frame->size());
        return;
    }
    uint8_t brightnessLevel = (*frame)[5];
    float brightness = static_cast<float>(brightnessLevel) / 255.0f;
    uint16_t duration = ((*frame)[6] << 8) | (*frame)[7];
    ESP_LOGI("StatusLightHandler", "Command 0x05 (ChangeBrightnessCommand): Brightness=%f, Duration=%d", brightness, duration);
    changeBrightness(brightness, duration);
}

void StatusLightHandler::updateTaskWrapper(void *args)
{
    auto *instance = static_cast<StatusLightHandler *>(args);
    instance->updateTask();
}

void StatusLightHandler::updateTask()
{
    while (true)
    {
        xSemaphoreTake(mutex, portMAX_DELAY);
        if (currentAnimation != nullptr)
        {
            currentAnimation->update(INTERVAL);
            currentAnimation->render();
            if (currentAnimation->isFinished())
            {
                if (nextAnimation == nullptr)
                {
                    currentAnimation = nullptr;
                }
                else
                {
                    currentAnimation = std::move(nextAnimation);
                    nextAnimation = nullptr;
                }
            }
        }
        xSemaphoreGive(mutex);
        vTaskDelay(pdMS_TO_TICKS(INTERVAL));
    }
}

void StatusLightHandler::processCommandQueueTaskWrapper(void *args)
{
    auto *instance = static_cast<StatusLightHandler *>(args);
    instance->processCommandQueueTask();
}

void StatusLightHandler::processCommandQueueTask()
{
    while (true)
    {
        std::shared_ptr<std::vector<uint8_t>> frame;
        if (xQueueReceive(commandQueueHandle, &frame, portMAX_DELAY) != pdTRUE)
        {
            continue;
        }

        uint8_t commandId = (*frame)[4];
        ESP_LOGI("StatusLightHandler", "Received command with id %d", commandId);
        switch (commandId)
        {
        case 0x01:
        {
            // ff 00 11 01 01 01 80 00 20 09 c4 40 00 00 01 b0 fe (single)
            // ff 00 11 01 01 01 80 00 20 09 c4 40 01 00 01 b1 fe (infinite)
            // ff 00 11 01 01 01 80 00 20 13 88 40 01 00 01 7f fe (infinite-slow)
            processStartAnimationCommand(frame);
            break;
        }
        case 0x02:
        {
            // FF 00 09 01 02 01 00 04 FE
            // FF 00 09 01 02 00 00 03 FE
            processStopAnimationCommand(frame);
            break;
        }
        case 0x03:
        {
            // FF 00 09 01 03 01 00 05 FE
            // FF 00 09 01 03 00 00 04 FE
            processSkipAnimationCommand(frame);
            break;
        }
        case 0x04:
        {
            // FF 00 0d 01 04 20 60 60 02 ff 01 e6 FE (blue-ish)
            // FF 00 0c 01 04 f0 60 00 02 ff 02 56 FE (yellow-ish)
            processChangeColorCommand(frame);
            break;
        }
        case 0x05:
        {
            // FF 00 0a 01 05 d0 01 f4 01 cb FE (80%)
            // FF 00 0a 01 05 30 01 f4 01 0d FE (18%)
            // FF 00 0a 01 05 10 01 f4 01 0b FE (6%)
            processChangeBrightnessCommand(frame);
            break;
        }
        default:
        {
            ESP_LOGW("StatusLightHandler", "Unknown command with id: %d", commandId);
            break;
        }
        }
    }
}

void StatusLightHandler::startAnimation(const StatusLightAnimationConfig &config, bool interruptCurrentAnimation)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    std::unique_ptr<StatusLightAnimation> animation;
    switch (config.type)
    {
    case StatusLightAnimationType::LOADING:
        animation = std::make_unique<LoadingAnimation>(config, statusLights);
        break;
    case StatusLightAnimationType::FADE_IN:
        animation = std::make_unique<FadeInAnimation>(config, statusLights);
        break;
    case StatusLightAnimationType::FADE_OUT:
        animation = std::make_unique<FadeOutAnimation>(config, statusLights);
        break;
    case StatusLightAnimationType::ERROR:
        animation = std::make_unique<ErrorAnimation>(config, statusLights);
        break;
    case StatusLightAnimationType::PULSE:
        animation = std::make_unique<PulseAnimation>(config, statusLights);
        break;
    default:
        xSemaphoreGive(mutex);
        ESP_LOGE("StatusLightHandler", "Unknown animation type");
        return;
    }
    if (currentAnimation == nullptr)
    {
        currentAnimation = std::move(animation);
    }
    else
    {
        if (interruptCurrentAnimation)
        {
            statusLights.setPixels(0, 0, 0);
            statusLights.show();
            currentAnimation = std::move(animation);
        }
        else
        {
            nextAnimation = std::move(animation);
            if (currentAnimation->isInfinite())
            {
                currentAnimation->setInfinite(false);
            }
        }
    }
    xSemaphoreGive(mutex);
}

void StatusLightHandler::stopAnimation(bool interruptCurrentAnimation)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (currentAnimation != nullptr)
    {
        if (interruptCurrentAnimation)
        {
            statusLights.setPixels(0, 0, 0);
            statusLights.show();
            currentAnimation = nullptr;
        }
        else
        {
            if (currentAnimation->isInfinite())
            {
                currentAnimation->setInfinite(false);
            }
        }
        if (nextAnimation != nullptr)
        {
            nextAnimation = nullptr;
        }
    }
    xSemaphoreGive(mutex);
}

void StatusLightHandler::skipAnimation(bool interruptCurrentAnimation)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (currentAnimation != nullptr)
    {
        if (interruptCurrentAnimation)
        {
            statusLights.setPixels(0, 0, 0);
            statusLights.show();
            if (nextAnimation == nullptr)
            {
                currentAnimation = nullptr;
            }
            else
            {
                currentAnimation = std::move(nextAnimation);
                nextAnimation = nullptr;
            }
        }
        else
        {
            if (currentAnimation->isInfinite())
            {
                currentAnimation->setInfinite(false);
            }
        }
    }
    xSemaphoreGive(mutex);
}

void StatusLightHandler::changeColor(uint8_t red, uint8_t green, uint8_t blue, uint16_t duration)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (currentAnimation != nullptr)
    {
        currentAnimation->changeColor(red, green, blue, duration);
        ESP_LOGI("StatusLightHandler", "Changing color to %d %d %d", red, green, blue);
    }
    xSemaphoreGive(mutex);
}

void StatusLightHandler::changeBrightness(float brightness, uint16_t duration)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (currentAnimation != nullptr)
    {
        currentAnimation->changeBrightness(brightness, duration);
        ESP_LOGI("StatusLightHandler", "Changing brightness to %f", brightness);
    }
    xSemaphoreGive(mutex);
}

QueueHandle_t &StatusLightHandler::getCommandQueue()
{
    return commandQueueHandle;
}