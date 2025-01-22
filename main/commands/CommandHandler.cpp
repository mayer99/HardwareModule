#include "CommandHandler.h"
#include <StatusLightAnimation.h>
#include <LoadingAnimation.h>

constexpr int UART_BUFFER_SIZE = 256;
constexpr int MAX_BUFFER_SIZE = 1000;
constexpr uint8_t START_BYTE = 0xFF;
constexpr uint8_t END_BYTE = 0xFE;
constexpr uart_port_t UART_NUM = UART_NUM_0;
constexpr uint8_t UPDATE_INTERVAL = 100;

CommandHandler::CommandHandler(StatusLightHandler &statusLightHandler) : statusLightHandler(statusLightHandler)
{
    initializeUart();
    xTaskCreate(
        CommandHandler::updateTaskWrapper,
        "CommandHandlerUpdateTask",
        25600,
        this,
        1,
        nullptr);
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

        if (buffer.size() + newDataLength > MAX_BUFFER_SIZE)
        {
            ESP_LOGW("UART", "Buffer overflow risk, clearing buffer");
            buffer.clear();
        }

        buffer.insert(buffer.end(), newDataBuffer, newDataBuffer + newDataLength);
        ESP_LOGI("UART", "Buffer size: %d", buffer.size());

        processBuffer();
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
        ESP_LOGI("UART", "Possible start index: %d", std::distance(buffer.begin(), it));

        // Ensure we have enough bytes for length and checksum
        if (std::distance(it, buffer.end()) < 5)
        {
            ESP_LOGI("UART", "Not enough length");
            break;
        }

        uint16_t length = (*(it + 1) << 8) | *(it + 2);
        if (std::distance(it, buffer.end()) < length)
        {
            ESP_LOGI("UART", "Not enough 2nd length");
            ++it; // Zum nächsten Byte gehen
            continue;
        }

        // Check end byte
        if (*(it + length - 1) != END_BYTE)
        {
            ESP_LOGI("UART", "Mismatched end byte");
            ++it; // Zum nächsten Byte gehen
            continue;
        }

        // Validate checksum
        uint64_t checksum = 0;
        for (auto checksumIt = it + 3; checksumIt != it + length - 3; ++checksumIt)
        {
            checksum += *checksumIt;
        }

        uint16_t calculatedChecksum = static_cast<uint16_t>(checksum & 0xFFFF);
        uint16_t packetChecksum = (*(it + length - 3) << 8) | *(it + length - 2);

        if (calculatedChecksum != packetChecksum)
        {
            ESP_LOGW("UART", "Checksum mismatch: calculated=%d, packet=%d", calculatedChecksum, packetChecksum);
            ++it; // Zum nächsten Byte gehen
            continue;
        }

        // Extract and process the frame
        std::vector<uint8_t> frame(it, it + length);
        processFrame(frame);

        // Move to the next frame
        it = it + length;
        newStartIt = it;
    }

    // Remove processed bytes
    if (std::distance(buffer.begin(), newStartIt) > 0)
    {
        buffer.erase(buffer.begin(), newStartIt);
    }
}

void CommandHandler::processFrame(const std::vector<uint8_t> &frame)
{
    // Convert frame to hex string for logging
    std::string hexString;
    for (uint8_t byte : frame)
    {
        char hex[3];
        sprintf(hex, "%02X", byte);
        hexString += hex;
    }
    ESP_LOGI("UART", "Frame: %s", hexString.c_str());

    uint8_t command = frame[3];
    switch (command)
    {
    case 0x01:
    {
        // ff 00 10 01 01 80 00 20 09 c4 40 00 00 01 af fe (single)
        // ff 00 10 01 01 80 00 20 09 c4 40 01 00 01 b0 fe (infinite)
        // ff 00 10 01 01 80 00 20 13 88 40 01 00 01 7e fe (infinite-slow)
        ESP_LOGI("UART", "Command 0x01");
        if (frame.size() != 16)
        {
            ESP_LOGW("UART", "Invalid frame size for command 0x01: %d", frame.size());
            return;
        }
        uint8_t typeId = frame[4];
        uint8_t red = frame[5];
        uint8_t green = frame[6];
        uint8_t blue = frame[7];
        uint16_t duration = (frame[8] << 8) | frame[9];
        uint8_t brightnessLevel = frame[10];
        float brightness = static_cast<float>(brightnessLevel) / 255.0f;
        bool infinite = static_cast<bool>(frame[11]);
        bool interruptCurrentAnimation = static_cast<bool>(frame[12]);

        ESP_LOGI("UART", "Command 0x01: Type=%d, Red=%d, Green=%d, Blue=%d, Duration=%d, Brightness=%f, Infinite=%d, InterruptCurrentAnimation=%d",
                 typeId, red, green, blue, duration, brightness, infinite, interruptCurrentAnimation);

        StatusLightAnimationType type;
        switch (typeId)
        {
        case 0x01:
            type = StatusLightAnimationType::LOADING;
            break;
        case 0x02:
            type = StatusLightAnimationType::FADE_IN;
            break;
        case 0x03:
            type = StatusLightAnimationType::FADE_OUT;
            break;
        case 0x04:
            type = StatusLightAnimationType::ERROR;
            break;
        case 0x05:
            type = StatusLightAnimationType::PULSE;
            break;
        default:
            ESP_LOGW("UART", "Unknown animation type: %d", typeId);
            return;
        }
        StatusLightAnimationConfig config = {
            .type = type,
            .red = red,
            .green = green,
            .blue = blue,
            .duration = duration,
            .brightness = brightness,
            .infinite = infinite};
        statusLightHandler.startAnimation(config, interruptCurrentAnimation);
        break;
    }
    case 0x02:
    {
        // FF 00 08 02 00 00 02 FE
        ESP_LOGI("UART", "Command 0x02");
        if (frame.size() != 8)
        {
            ESP_LOGW("UART", "Invalid frame size for command 0x02: %d", frame.size());
            return;
        }
        bool interruptCurrentAnimation = static_cast<bool>(frame[4]);
        ESP_LOGI("UART", "Command 0x02: InterruptCurrentAnimation=%d", interruptCurrentAnimation);
        statusLightHandler.stopAnimation(interruptCurrentAnimation);
        break;
    }
    case 0x03:
    {
        // FF 00 08 03 00 00 03 FE
        ESP_LOGI("UART", "Command 0x03");
        if (frame.size() != 8)
        {
            ESP_LOGW("UART", "Invalid frame size for command 0x03: %d", frame.size());
            return;
        }
        bool interruptCurrentAnimation = static_cast<bool>(frame[4]);
        ESP_LOGI("UART", "Command 0x03: InterruptCurrentAnimation=%d", interruptCurrentAnimation);
        statusLightHandler.skipAnimation(interruptCurrentAnimation);
        break;
    }
    case 0x04:
    {
        ESP_LOGI("UART", "Command 0x04");
        // FF 00 0c 04 20 60 60 02 ff 01 e5 FE (blue-ish)
        // FF 00 0c 04 f0 60 00 02 ff 02 55 FE (yellow-ish)
        if (frame.size() != 12)
        {
            ESP_LOGW("UART", "Invalid frame size for command 0x04: %d", frame.size());
            return;
        }
        uint8_t red = frame[4];
        uint8_t green = frame[5];
        uint8_t blue = frame[6];
        uint16_t duration = (frame[7] << 8) | frame[8];

        ESP_LOGI("UART", "Command 0x04: Red=%d, Green=%d, Blue=%d, Duration=%d", red, green, blue, duration);
        statusLightHandler.changeColor(red, green, blue, duration);
        break;
    }
    case 0x05:
    {
        ESP_LOGI("UART", "Command 0x05");
        // FF 00 0a 05 d0 01 f4 01 ca FE (80%)
        // FF 00 0a 05 30 01 f4 01 0c FE (18%)
        // FF 00 0a 05 10 01 f4 01 0a FE (6%)
        if (frame.size() != 10)
        {
            ESP_LOGW("UART", "Invalid frame size for command 0x05: %d", frame.size());
            return;
        }
        uint8_t brightnessLevel = frame[4];
        float brightness = static_cast<float>(brightnessLevel) / 255.0f;
        uint16_t duration = (frame[5] << 8) | frame[6];
        ESP_LOGI("UART", "Command 0x05: Brightness=%f, Duration=%d", brightness, duration);
        statusLightHandler.changeBrightness(brightness, duration);
        break;
    }
    default:
    {
        ESP_LOGW("UART", "Unknown command: %d", command);
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
