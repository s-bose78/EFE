#include <stdio.h>
#include <stdlib.h>
#include <string.h> //Requires by memset
#include <esp_err.h>
#include "led_gpio.h"
#include "esp_log.h"

static const char *TAG = "LED";
void gpio_configure(int pin)
{
    // Configure the I/O pin for the LED
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;          // Disable interrupt
    io_conf.pin_bit_mask = (1ULL << pin);      // Select GPIO pin
    io_conf.mode = GPIO_MODE_OUTPUT;               // Set pin as output
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;     // No pull-up resistor
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // No pull-down resistor
    gpio_config(&io_conf);                         // Configure GPIO   
}

void set_gpio(int pin, uint32_t state)
{
    if (state)
        ESP_LOGI(TAG, "%ld ON", state);
    else
        ESP_LOGI(TAG, "%ld OFF", state);
    gpio_set_level(pin, state);
}