#include <StatusLightController.h>
#include "LoadingAnimation.h"
#include "PulseAnimation.h"
#include "FadeInAnimation.h"
#include "FadeOutAnimation.h"
#include <CommandHandler.h>
#include "StatusLightCommand.h"
#include <freertos/queue.h>

StatusLightHandler::StatusLightHandler()
{
    mutex = xSemaphoreCreateMutex();
    xTaskCreate(
        StatusLightController::updateTaskWrapper,
        "StatusLightControllerUpdateTask",
        25600,
        this,
        1,
        nullptr);
}

void StatusLightHandler::updateTaskWrapper(void *args)
{
    auto *controller = static_cast<StatusLightController *>(args);
    controller->updateTask();
}

void StatusLightHandler::updateTask()
{
    while (true)
    {
        xSemaphoreTake(mutex, portMAX_DELAY);
        if (currentAnimation == nullptr)
        {
            return;
        }
        currentAnimation->update(INTERVAL);
        currentAnimation->render();
        if (currentAnimation->isFinished())
        {
            ESP_LOGI(TAG, "currentAnimation finished");
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

void StatusLightController::sendCommand(Command command)
{
    command.data = std::vector<int>{1, 2, 3}; // Dynamische Daten
    xQueueSend(commandQueue, &command, portMAX_DELAY);
}

void StatusLightController::skipAnimation(bool interruptCurrentAnimation)
{
    if (currentAnimation == nullptr)
    {
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
}

void StatusLightController::stopAnimation(bool interruptCurrentAnimation)
{
    if (currentAnimation == nullptr)
    {
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
}

void StatusLightController::scheduleAnimation(std::unique_ptr<StatusLightAnimation> animation, bool interruptCurrentAnimation)
{
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
}

void StatusLightController::test()
{
    StatusLightAnimationConfig config = {
        .red = 255,
        .green = 0,
        .blue = 0,
        .duration = 5000, // Dauer der Animation in ms
        .brightness = 0.8f,
        .infinite = true // Endlosschleife
    };
    auto loadingAnimation = std::make_unique<LoadingAnimation>(config, &statusLights);
    scheduleAnimation(std::move(loadingAnimation), true);
}

void StatusLightController::schedulePulseAnimation()
{
    StatusLightAnimationConfig config = {
        .red = 0,
        .green = 255,
        .blue = 0,
        .duration = 2000, // Dauer der Animation in ms
        .brightness = 0.8f,
        .infinite = true // Endlosschleife
    };
    auto loadingAnimation = std::make_unique<LoadingAnimation>(config, &statusLights);
    scheduleAnimation(std::move(loadingAnimation), true);
}