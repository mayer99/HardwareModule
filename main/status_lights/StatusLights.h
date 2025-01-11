#include <driver/rmt_types.h>
#include <driver/rmt_tx.h>
#include "freertos/FreeRTOS.h"
#include <esp_log.h>

#ifndef STATUS_LIGHTS_H
#define STATUS_LIGHTS_H

#define LED_STRIP_RESOLUTION_HZ 10000000
#define LED_STRIP_GPIO_NUM GPIO_NUM_13
#define LED_COUNT 24
#define MAX_BRIGHTNESS 1.0f

#define TAG "StatusLights"

class StatusLights
{
private:
    rmt_channel_handle_t rmt_channel;
    rmt_tx_channel_config_t rmt_channel_config;
    rmt_encoder_handle_t rmt_encoder;
    rmt_simple_encoder_config_t rmt_encoder_config;
    rmt_transmit_config_t rmt_tx_config;
    uint8_t buffer[LED_COUNT * 3];
    static rmt_symbol_word_t rmt_symbol_zero;
    static rmt_symbol_word_t rmt_symbol_one;
    static rmt_symbol_word_t rmt_symbol_reset;
    static size_t encoder_callback(const void *data, size_t data_size, size_t symbols_written, size_t symbols_free, rmt_symbol_word_t *symbols, bool *done, void *arg);

public:
    StatusLights();
    void setPixel(int index, uint8_t red, uint8_t green, uint8_t blue);
    void setPixels(uint8_t red, uint8_t green, uint8_t blue);
    void setPixel(int index, uint8_t red, uint8_t green, uint8_t blue, float brightness);
    void setPixels(uint8_t red, uint8_t green, uint8_t blue, float brightness);
    void show();
    int getPixelCount();
};

#endif