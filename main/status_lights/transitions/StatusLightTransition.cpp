#include <StatusLightTransition.h>

StatusLightTransition::StatusLightTransition(int duration)
    : duration(duration)
{
}

void StatusLightTransition::update(int deltaTime)
{
    elapsedTime += deltaTime;
    if (elapsedTime >= duration)
    {
        finished = true;
    }
}

bool StatusLightTransition::isFinished()
{
    return finished;
}