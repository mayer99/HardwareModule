#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <cstring>
#include <algorithm>
#include <vector>
#include <string>
#include "StatusLightHandler.h"


enum class CommandHandlerState
{
    INITIAL,
    ACTIVE,
    ERROR
};

class CommandHandler
{
private:
    std::vector<uint8_t> buffer;
    StatusLightHandler &statusLightHandler;
    SemaphoreHandle_t mutex;
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point lastKeepAliveTime = std::chrono::steady_clock::now();
    CommandHandlerState state = CommandHandlerState::INITIAL;
    void initializeUart();
    static void updateTaskWrapper(void *args);
    void updateTask();
    static void stateMonitorTaskWrapper(void *args);
    void stateMonitorTask();
    void processBuffer();
    void processFrame(const std::vector<uint8_t> &frame);

public:
    CommandHandler(StatusLightHandler &statusLightHandler);
};

#endif
