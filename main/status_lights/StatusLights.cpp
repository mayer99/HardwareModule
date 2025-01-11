#include "StatusLights.h"

rmt_symbol_word_t StatusLights::rmt_symbol_zero;
rmt_symbol_word_t StatusLights::rmt_symbol_one;
rmt_symbol_word_t StatusLights::rmt_symbol_reset;

size_t StatusLights::encoder_callback(const void *data, size_t data_size,
                                      size_t symbols_written, size_t symbols_free,
                                      rmt_symbol_word_t *symbols, bool *done, void *arg)
{
    if (symbols_free < 8)
    {
        return 0;
    }
    size_t data_pos = symbols_written / 8;
    uint8_t *data_bytes = (uint8_t *)data;
    if (data_pos < data_size)
    {
        // Encode a byte
        size_t symbol_pos = 0;
        for (int bitmask = 0x80; bitmask != 0; bitmask >>= 1)
        {
            if (data_bytes[data_pos] & bitmask)
            {
                symbols[symbol_pos++] = rmt_symbol_one;
            }
            else
            {
                symbols[symbol_pos++] = rmt_symbol_zero;
            }
        }
        // We're done; we should have written 8 symbols.
        return symbol_pos;
    }
    else
    {
        // All bytes already are encoded.
        // Encode the reset, and we're done.
        symbols[0] = rmt_symbol_reset;
        *done = 1; // Indicate end of the transaction.
        return 1;  // we only wrote one symbol
    }
}

StatusLights::StatusLights()
{
    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_channel_config = {};
    rmt_channel_config.clk_src = RMT_CLK_SRC_DEFAULT; // select source clock
    rmt_channel_config.gpio_num = LED_STRIP_GPIO_NUM;
    rmt_channel_config.mem_block_symbols = 64; // increase the block size can make the LED less flickering
    rmt_channel_config.resolution_hz = LED_STRIP_RESOLUTION_HZ;
    rmt_channel_config.trans_queue_depth = 4; // set the number of transactions that can be pending in the background
    ESP_ERROR_CHECK(rmt_new_tx_channel(&rmt_channel_config, &rmt_channel));

    rmt_encoder_config = {};
    rmt_encoder_config.callback = encoder_callback;
    ESP_ERROR_CHECK(rmt_new_simple_encoder(&rmt_encoder_config, &rmt_encoder));

    rmt_symbol_zero.level0 = 1;
    rmt_symbol_zero.duration0 = static_cast<uint32_t>(0.3 * LED_STRIP_RESOLUTION_HZ / 1000000); // T0H=0.3us
    rmt_symbol_zero.level1 = 0;
    rmt_symbol_zero.duration1 = static_cast<uint32_t>(0.9 * LED_STRIP_RESOLUTION_HZ / 1000000); // T0L=0.9us

    // Initialisierung von ws2812_one
    rmt_symbol_one.level0 = 1;
    rmt_symbol_one.duration0 = static_cast<uint32_t>(0.9 * LED_STRIP_RESOLUTION_HZ / 1000000); // T1H=0.9us
    rmt_symbol_one.level1 = 0;
    rmt_symbol_one.duration1 = static_cast<uint32_t>(0.3 * LED_STRIP_RESOLUTION_HZ / 1000000); // T1L=0.3us

    // Initialisierung von ws2812_reset
    uint32_t reset_ticks = static_cast<uint32_t>(LED_STRIP_RESOLUTION_HZ / 1000000 * 50 / 2);
    rmt_symbol_reset.level0 = 1;
    rmt_symbol_reset.duration0 = reset_ticks; // 50us / 2
    rmt_symbol_reset.level1 = 0;
    rmt_symbol_reset.duration1 = reset_ticks;

    rmt_tx_config = {};
    rmt_tx_config.loop_count = 0; // no transfer loop
}
void StatusLights::setPixel(int index, uint8_t red, uint8_t green, uint8_t blue)
{
    buffer[index * 3 + 0] = red;
    buffer[index * 3 + 1] = green;
    buffer[index * 3 + 2] = blue;
}
void StatusLights::setPixels(uint8_t red, uint8_t green, uint8_t blue)
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        buffer[i * 3 + 0] = green;
        buffer[i * 3 + 1] = red;
        buffer[i * 3 + 2] = blue;
    }
}
void StatusLights::setPixel(int index, uint8_t red, uint8_t green, uint8_t blue, float brightness)
{
    buffer[index * 3 + 0] = static_cast<uint8_t>(green * brightness);
    buffer[index * 3 + 1] = static_cast<uint8_t>(red * brightness);
    ;
    buffer[index * 3 + 2] = static_cast<uint8_t>(blue * brightness);
    ;
}
void StatusLights::setPixels(uint8_t red, uint8_t green, uint8_t blue, float brightness)
{
    ESP_LOGI(TAG, "brightness: %f", brightness);
    for (int i = 0; i < LED_COUNT; i++)
    {
        auto r = static_cast<uint8_t>(red * brightness);
        ESP_LOGI(TAG, "r: %d", r);

        buffer[i * 3 + 0] = static_cast<uint8_t>(green * brightness);
        buffer[i * 3 + 1] = r;
        buffer[i * 3 + 2] = static_cast<uint8_t>(blue * brightness);
    }
}
void StatusLights::show()
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        // ESP_LOGI(TAG, "Pixel %d: %d %d %d", i, buffer[i * 3 + 0], buffer[i * 3 + 1], buffer[i * 3 + 2]);
    }
    ESP_ERROR_CHECK(rmt_enable(rmt_channel));
    ESP_ERROR_CHECK(rmt_transmit(rmt_channel, rmt_encoder, buffer, sizeof(buffer), &rmt_tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(rmt_channel, portMAX_DELAY));
    ESP_ERROR_CHECK(rmt_disable(rmt_channel));
};

int StatusLights::getPixelCount()
{
    return LED_COUNT;
};