#include "CommandHandler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>
#include <algorithm>

CommandHandler::CommandHandler()
{
    initializeUart();
    xTaskCreate(updateTask, "UpdateTask", 25600, this, 5, NULL);
}

void CommandHandler::updateTask(void *args)
{
    std::vector<uint8_t> buffer;
    while (1)
    {
        uint8_t newDataBuffer[256];
        int newDataLength = uart_read_bytes(UART_NUM, newDataBuffer, 256, pdMS_TO_TICKS(100));
        if (newDataLength == 0)
        {
            continue;
        }

        ESP_LOGI("UART", "Received %d bytes", newDataLength);
        if (buffer.size() > 1000)
        {
            ESP_LOGI("UART", "Buffer contains more than 1000 bytes, discarding");
            buffer.clear();
        }
        buffer.insert(buffer.end(), newDataBuffer, newDataBuffer + newDataLength);

        for (int i = 0; i < buffer.size(); i++)
        {
            ESP_LOGI("UART", "Reading byte %d", buffer[i]);
            if (buffer[i] != START_BYTE)
            {
                ESP_LOGI("UART", "Does not match START_BYTE");
                continue;
            }
            if (buffer.size() < i + 5)
            {
                ESP_LOGI("UART", "Remaining buffer too small");
                break;
            }
            uint16_t length = static_cast<uint16_t>((buffer[i + 1] << 8) | buffer[i + 2]);
            ESP_LOGI("UART", "Reading length of %d", length);
            if (buffer.size() < i + length) // buffer.size() - 1 < i + length - 1 because we need to account for the zero-based index on the left and the missing start_byte on the right
            {
                ESP_LOGI("UART", "Remaining buffer too small for length value");
                continue;
            }
            if (buffer[i + length - 1] != END_BYTE)
            {
                ESP_LOGI("UART", "Does not match END_BYTE");
                continue;
            }

            uint64_t checksum = 0;
            for (uint16_t j = 3; j < length - 3; j++)
            {
                checksum += buffer[i + j];
            }
            uint16_t low_bytes = static_cast<uint16_t>(checksum & 0xFFFF);
            ESP_LOGI("UART", "Checksum: %d", low_bytes);

            // Ende abschneiden
            uint16_t packetChecksum = static_cast<uint16_t>((buffer[i + length - 3] << 8) | buffer[i + length - 2]);
            ESP_LOGI("UART", "packet Checksum: %d", packetChecksum);
            if (checksum != packetChecksum)
            {
                ESP_LOGI("UART", "Checksum does not match");
                continue;
            }
            ESP_LOGI("UART", "End");

            std::vector<uint8_t> subset(buffer.begin() + i + 3, buffer.begin() + i + length - 3);
            // convert subset to hex string
            std::string hexString;
            for (uint16_t j = 0; j < subset.size(); j++)
            {
                char hex[3];
                sprintf(hex, "%02X", subset[j]);
                hexString += hex;
            }
            ESP_LOGI("UART", "Hex string: %s", hexString.c_str());
        }
    }
};

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
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUFFER_SIZE, 0, 0, NULL, 0));
}