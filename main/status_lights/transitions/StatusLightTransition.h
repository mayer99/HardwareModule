#ifndef STATUSLIGHTTRANSITION_H
#define STATUSLIGHTTRANSITION_H

#include <algorithm>

class StatusLightTransition
{
private:
protected:
    bool finished = false;
    int elapsedTime = 0;
    int duration;
public:
    StatusLightTransition(int duration);
    virtual void update(int deltaTime);
    bool isFinished();
};

#endif