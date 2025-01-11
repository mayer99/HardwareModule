#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include "driver/uart.h"
#include "esp_log.h"
#include <vector>
#include <string>
#include <chrono>

#define UART_NUM UART_NUM_0
#define BUFFER_SIZE 1024
#define START_BYTE 0xFF
#define END_BYTE 0xFE
#define UPDATE_INTERVAL 100

class CommandHandler
{
private:
    void initializeUart();

public:
    CommandHandler();
    static void updateTask(void* args);
};

#endif