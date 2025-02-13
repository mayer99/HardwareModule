#include "CommandHandler.h"
#include <StatusLightAnimation.h>
#include <LoadingAnimation.h>

constexpr int UART_BUFFER_SIZE = 256;
constexpr int MAX_BUFFER_SIZE = 1000;
constexpr uint8_t START_BYTE = 0xFF;
constexpr uint8_t END_BYTE = 0xFE;
constexpr uart_port_t UART_NUM = UART_NUM_0;
constexpr uint8_t UPDATE_INTERVAL = 100;
constexpr uint32_t TIMEOUT_INITIAL = 120000;
constexpr uint32_t TIMEOUT_KEEPALIVE = 60000;

CommandHandler::CommandHandler(StatusLightHandler &statusLightHandler) : statusLightHandler(statusLightHandler)
{
    initializeUart();

    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr)
    {
        ESP_LOGE("CommandHandler", "Failed to create mutex");
        return;
    }

    xTaskCreate(CommandHandler::updateTaskWrapper, "CommandHandlerUpdateTask", 25600, this, 1, nullptr);
    xTaskCreate(CommandHandler::stateMonitorTaskWrapper, "CommandHandlerStateMonitorTask", 8192, this, 1, nullptr);

    StatusLightAnimationConfig config = {
        .type = StatusLightAnimationType::PULSE,
        .red = 255,
        .green = 255,
        .blue = 255,
        .duration = 2000,
        .brightness = 0.5f,
        .infinite = true};
    statusLightHandler.startAnimation(config, false);
}

void CommandHandler::updateTaskWrapper(void *args)
{
    auto *instance = static_cast<CommandHandler *>(args);
    instance->updateTask();
}

void CommandHandler::updateTask()
{
    while (1)
    {
        uint8_t newDataBuffer[UART_BUFFER_SIZE];
        int newDataLength = uart_read_bytes(UART_NUM, newDataBuffer, UART_BUFFER_SIZE, pdMS_TO_TICKS(UPDATE_INTERVAL));

        if (newDataLength <= 0)
        {
            continue;
        }

        xSemaphoreTake(mutex, portMAX_DELAY);
        if (state == CommandHandlerState::ERROR)
        {
            ESP_LOGW("CommandHandler", "Ignoring data in error state");
            xSemaphoreGive(mutex);
            continue;
        }

        if (buffer.size() + newDataLength > MAX_BUFFER_SIZE)
        {
            ESP_LOGW("CommandHandler", "Buffer overflow risk, clearing buffer");
            buffer.clear();
        }

        buffer.insert(buffer.end(), newDataBuffer, newDataBuffer + newDataLength);
        ESP_LOGI("CommandHandler", "Buffer size: %d", buffer.size());

        processBuffer();
        xSemaphoreGive(mutex);
    }
}

void CommandHandler::stateMonitorTaskWrapper(void *args)
{
    auto *instance = static_cast<CommandHandler *>(args);
    instance->stateMonitorTask();
}

void CommandHandler::stateMonitorTask()
{
    while (1)
    {
        xSemaphoreTake(mutex, portMAX_DELAY);
        switch (state)
        {
        case CommandHandlerState::INITIAL:
        {
            auto currentTime = std::chrono::steady_clock::now();
            auto startDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
            if (startDuration.count() > TIMEOUT_INITIAL)
            {
                ESP_LOGI("CommandHandler", "Initial timeout reached");
                state = CommandHandlerState::ERROR;
                StatusLightAnimationConfig config = {
                    .type = StatusLightAnimationType::ERROR,
                    .red = 255,
                    .green = 0,
                    .blue = 0,
                    .duration = 2000,
                    .brightness = 0.4f,
                    .infinite = true};
                statusLightHandler.startAnimation(config, false);
            };
            break;
        }
        case CommandHandlerState::ACTIVE:
        {
            auto currentTime = std::chrono::steady_clock::now();
            auto lastKeepAliveDuration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastKeepAliveTime);
            if (lastKeepAliveDuration.count() > TIMEOUT_KEEPALIVE)
            {
                ESP_LOGI("CommandHandler", "KeepAlive timeout reached");
                state = CommandHandlerState::ERROR;
                StatusLightAnimationConfig config = {
                    .type = StatusLightAnimationType::ERROR,
                    .red = 255,
                    .green = 0,
                    .blue = 0,
                    .duration = 2000,
                    .brightness = 0.4f,
                    .infinite = true};
                statusLightHandler.startAnimation(config, false);
            }
            break;
        }
        default:
        {
            break;
        }
        }
        xSemaphoreGive(mutex);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void CommandHandler::processBuffer()
{
    auto it = buffer.begin();
    auto newStartIt = buffer.begin();

    while (it != buffer.end())
    {
        // Find the start of a frame
        it = std::find(it, buffer.end(), START_BYTE);
        if (it == buffer.end())
        {
            break;
        }
        ESP_LOGI("CommandHandler", "Possible start index: %d", std::distance(buffer.begin(), it));

        if (std::distance(it, buffer.end()) < 7)
        {
            ESP_LOGI("CommandHandler", "Not enough length to contain length value, handlerId, commandId, checksum and endbyte");
            break;
        }

        uint16_t length = (*(it + 1) << 8) | *(it + 2);
        if (std::distance(it, buffer.end()) < length)
        {
            ESP_LOGI("CommandHandler", "Not enough length to match endbyte");
            ++it;
            continue;
        }

        if (*(it + length - 1) != END_BYTE)
        {
            ESP_LOGI("CommandHandler", "Mismatched end byte");
            ++it;
            continue;
        }

        uint64_t checksum = 0;
        for (auto checksumIt = it + 3; checksumIt != it + length - 3; ++checksumIt)
        {
            checksum += *checksumIt;
        }

        uint16_t calculatedChecksum = static_cast<uint16_t>(checksum & 0xFFFF);
        uint16_t packetChecksum = (*(it + length - 3) << 8) | *(it + length - 2);

        if (calculatedChecksum != packetChecksum)
        {
            ESP_LOGW("CommandHandler", "Checksum mismatch: calculated=%d, packet=%d", calculatedChecksum, packetChecksum);
            ++it;
            continue;
        }

        std::vector<uint8_t> frame(it, it + length);
        processFrame(frame);

        it = it + length;
        newStartIt = it;
    }

    if (std::distance(buffer.begin(), newStartIt) > 0)
    {
        buffer.erase(buffer.begin(), newStartIt);
    }
}

void CommandHandler::processFrame(const std::vector<uint8_t> &frame)
{
    std::string hexString;
    for (uint8_t byte : frame)
    {
        char hex[3];
        sprintf(hex, "%02X", byte);
        hexString += hex;
    }
    ESP_LOGI("CommandHandler", "Frame: %s", hexString.c_str());

    if (state == CommandHandlerState::INITIAL)
    {
        ESP_LOGI("CommandHandler", "Received first command, switching to active state");
        state = CommandHandlerState::ACTIVE;
        lastKeepAliveTime = std::chrono::steady_clock::now();
        statusLightHandler.stopAnimation(false);
    }

    uint8_t handlerId = frame[3];
    switch (handlerId)
    {
    case 0x01:
    {
        std::shared_ptr<std::vector<uint8_t>> framePtr = std::make_shared<std::vector<uint8_t>>(std::move(frame));

        if (xQueueSend(statusLightHandler.getCommandQueue(), &framePtr, portMAX_DELAY) != pdTRUE)
        {
            ESP_LOGE("CommandHandler", "Failed to send frame to the CommandQueue of StatusLightHandler");
        }
        else
        {
            ESP_LOGI("CommandHandler", "Frame successfully sent to the CommandQueue of StatusLightHandler");
        }
        break;
    }
    case 0x02:
    {
        uint8_t commandId = frame[4];
        ESP_LOGI("CommandHandler", "Received command with id %d", commandId);
        switch (commandId)
        {
        case 0x01:
        {
            // FF 00 08 02 01 00 03 FE
            if (frame.size() != 8)
            {
                ESP_LOGW("CommandHandler", "Invalid frame size for command 0x01 (KeepAliveResetCommand): %d", frame.size());
                return;
            }
            lastKeepAliveTime = std::chrono::steady_clock::now();
            ESP_LOGI("CommandHandler", "Command 0x01 (KeepAliveResetCommand): KeepAlive reset");
            break;
        }
        case 0x02:
        {
            // FF 00 08 02 02 00 04 FE
            if (frame.size() != 8)
            {
                ESP_LOGW("CommandHandler", "Invalid frame size for command 0x02 (RebootCommand): %d", frame.size());
                return;
            }
            state = CommandHandlerState::INITIAL;
            startTime = std::chrono::steady_clock::now();
            StatusLightAnimationConfig config = {
                .type = StatusLightAnimationType::PULSE,
                .red = 255,
                .green = 255,
                .blue = 255,
                .duration = 2000,
                .brightness = 0.4f,
                .infinite = true};
            statusLightHandler.startAnimation(config, true);
            ESP_LOGI("CommandHandler", "Command 0x02 (RebootCommand): Reboot");
            break;
        }
        default:
        {
            ESP_LOGW("CommandHandler", "Unknown command: %d", commandId);
            break;
        }
        }
        break;
    }
    default:
    {
        ESP_LOGW("CommandHandler", "Unknown handler: %d", handlerId);
        break;
    }
    }
}

void CommandHandler::initializeUart()
{
    uart_config_t uart_config = {};
    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_APB;

    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, MAX_BUFFER_SIZE, 0, 0, NULL, 0));
}
