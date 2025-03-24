/* Capacative Touch Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "driver/touch_sensor.h"

static const char *TAG = "touch";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;

void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    
    /* Configure the I/O pin for the LED */
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;          // Disable interrupt
    io_conf.pin_bit_mask = (1ULL << BLINK_GPIO);      // Select GPIO pin
    io_conf.mode = GPIO_MODE_OUTPUT;               // Set pin as output
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;     // No pull-up resistor
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // No pull-down resistor
    gpio_config(&io_conf);                         // Configure GPIO   
}

esp_err_t configure_touch(void)
{
    esp_err_t ret;

    /* Initialize touch pad peripheral. */
    ret = touch_pad_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Touch sensor initialization failed\n");
        return ret;
    }

    /* Configure the touch sensor thresholds */
    ret = touch_pad_config(TOUCH_PAD_NUM14);  // Configure pad 0 with threshold 0
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure touch pad\n");
        return ret;
    }
    /* Denoise setting at TouchSensor 0. */
    touch_pad_denoise_t denoise = {
        /* The bits to be cancelled are determined according to the noise level. */
        .grade = TOUCH_PAD_DENOISE_BIT4,
        .cap_level = TOUCH_PAD_DENOISE_CAP_L4,
    };
    ret = touch_pad_denoise_set_config(&denoise);
    if (ret) {
        ESP_LOGE(TAG, "Failed to set denoise config\n");
        return ret;
    }

    ret = touch_pad_denoise_enable();
    if (ret) {
        ESP_LOGE(TAG, "Failed to enable denoise\n");
        return ret;
    }

    /* Enable touch sensor clock. Work mode is "timer trigger". */
    ret = touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    if (ret) {
        ESP_LOGE(TAG, "Failed to set fsm mode\n");
        return ret;
    }

    ret = touch_pad_fsm_start();
    if (ret) {
        ESP_LOGE(TAG, "Failed to start fsm\n");
        return ret;
    }

    return ESP_OK;
}

void initial_debounce(void) {
    int i;
    uint32_t touch_value;

    for (i=0; i < 10; i++) {
        touch_pad_read_raw_data(TOUCH_PAD_NUM14, &touch_value);
        vTaskDelay(1000/ portTICK_PERIOD_MS);
    }
}

void touch_task(void *arg) {
    esp_err_t ret;
    uint32_t touch_value;

    /* Start measuring the touch pads */
    while (1) {
        ret = touch_pad_read_raw_data(TOUCH_PAD_NUM14, &touch_value);
        if (ret == ESP_OK) {
            /* printf("Touch value: %ld\n", touch_value); */
            if (touch_value > 100000) {
                /* Toggle the LED state */
                s_led_state = !s_led_state;

                /* Set the GPIO level according to the state (LOW or HIGH)*/
                gpio_set_level(BLINK_GPIO, s_led_state);

                ESP_LOGI(TAG, "Turn LED %s", s_led_state?"ON": "OFF");
                /* debounce time of 3 secs */
                vTaskDelay(3000/ portTICK_PERIOD_MS);                
            }
        } else {
            ESP_LOGE(TAG, "read_raw_val failed");
            ESP_ERROR_CHECK(ret);
        }
        vTaskDelay(1000/ portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    /* Configure the peripheral according to the LED type */
    configure_led();
    ESP_ERROR_CHECK(configure_touch());
    ESP_LOGI(TAG, "Touch Configuration Complete");
    initial_debounce();
    ESP_LOGI(TAG, "Initial Debounce Complete");
    ESP_LOGI(TAG, "Creating touch task");
    xTaskCreate(touch_task, "touch_task", 4096, NULL, 1, NULL);
}
