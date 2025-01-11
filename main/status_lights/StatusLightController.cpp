#include <StatusLightController.h>
#include "LoadingAnimation.h"
#include "PulseAnimation.h"
#include "FadeInAnimation.h"
#include "FadeOutAnimation.h"

StatusLightController::StatusLightController()
{
    xTaskCreate(updateTask, "updateTask", 4096, NULL, 5, NULL);
}

void StatusLightController::updateTask(void* args)
{
    StatusLights statusLights;
    std::unique_ptr<StatusLightAnimation> currentAnimation;
    std::unique_ptr<StatusLightAnimation> nextAnimation;
    while (true)
    {
        if (currentAnimation == nullptr)
        {
            return;
        }
        currentAnimation->update(deltaTime);
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
        vTaskDelay(INTERVAL);
    }
}

void StatusLightController::update()
{
    std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
    int deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastExecutionTime).count();
    if (deltaTime > INTERVAL)
    {
        lastExecutionTime = currentTime;
        
    }
};

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
        .infinite = true  // Endlosschleife
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
        .infinite = true  // Endlosschleife
    };
    auto loadingAnimation = std::make_unique<LoadingAnimation>(config, &statusLights);
    scheduleAnimation(std::move(loadingAnimation), true);
}