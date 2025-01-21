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
    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr)
    {
        ESP_LOGE("StatusLightHandler", "Failed to create mutex");
        return;
    }
    xTaskCreate(updateTaskWrapper, "StatusLightHandlerUpdateTask", 8128, this, 5, nullptr);
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
        if (currentAnimation == nullptr)
        {
            xSemaphoreGive(mutex);
            vTaskDelay(pdMS_TO_TICKS(INTERVAL));
            continue;
        }
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
        xSemaphoreGive(mutex);
        vTaskDelay(pdMS_TO_TICKS(INTERVAL));
    }
}

void StatusLightHandler::skipAnimation(bool interruptCurrentAnimation)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (currentAnimation == nullptr)
    {
        xSemaphoreGive(mutex);
        return;
    }

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
    xSemaphoreGive(mutex);
}

void StatusLightHandler::stopAnimation(bool interruptCurrentAnimation)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (currentAnimation == nullptr)
    {
        xSemaphoreGive(mutex);
        return;
    }

    if (interruptCurrentAnimation)
    {
        statusLights.setPixels(0, 0, 0);
        statusLights.show();
        currentAnimation = nullptr;
        if (nextAnimation != nullptr)
        {
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
    xSemaphoreGive(mutex);
}

void StatusLightHandler::startAnimation(const StatusLightAnimationConfig &config, bool interruptCurrentAnimation)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    auto animation = createAnimation(config);
    if (animation == nullptr)
    {
        xSemaphoreGive(mutex);
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
        }
    }
    xSemaphoreGive(mutex);
}

void StatusLightHandler::changeColor(uint8_t red, uint8_t green, uint8_t blue, uint16_t duration)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (currentAnimation == nullptr)
    {
        xSemaphoreGive(mutex);
        return;
    }
    currentAnimation->changeColor(red, green, blue, duration);
    ESP_LOGI("StatusLightHandler", "Changing color to %d %d %d", red, green, blue);
    xSemaphoreGive(mutex);
}

void StatusLightHandler::changeBrightness(float brightness, uint16_t duration)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (currentAnimation == nullptr)
    {
        xSemaphoreGive(mutex);
        return;
    }
    currentAnimation->changeBrightness(brightness, duration);
    ESP_LOGI("StatusLightHandler", "Changing brightness to %f", brightness);
    xSemaphoreGive(mutex);
}

std::unique_ptr<StatusLightAnimation> StatusLightHandler::createAnimation(const StatusLightAnimationConfig &config)
{
    switch (config.type)
    {
    case StatusLightAnimationType::LOADING:
        return std::make_unique<LoadingAnimation>(config, statusLights);
    case StatusLightAnimationType::FADE_IN:
        return std::make_unique<FadeInAnimation>(config, statusLights);
    case StatusLightAnimationType::FADE_OUT:
        return std::make_unique<FadeOutAnimation>(config, statusLights);
    case StatusLightAnimationType::ERROR:
        return std::make_unique<ErrorAnimation>(config, statusLights);
    case StatusLightAnimationType::PULSE:
        return std::make_unique<PulseAnimation>(config, statusLights);
    default:
        ESP_LOGE("StatusLightHandler", "Unknown animation type");
        return nullptr;
    }
}
