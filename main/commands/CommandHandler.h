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


class CommandHandler
{
private:
    std::vector<uint8_t> buffer;
    StatusLightHandler &statusLightHandler;
    void initializeUart();
    static void updateTaskWrapper(void *args);
    void updateTask();
    void processBuffer();
    void processFrame(const std::vector<uint8_t> &frame);

public:
    CommandHandler(StatusLightHandler &statusLightHandler);
};

#endif