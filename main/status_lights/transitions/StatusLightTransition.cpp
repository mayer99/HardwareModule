#include <StatusLightTransition.h>

StatusLightTransition::StatusLightTransition(int duration)
    : duration(duration)
{
}

void StatusLightTransition::update(int deltaTime)
{
    
}

bool StatusLightTransition::isFinished()
{
    return finished;
}