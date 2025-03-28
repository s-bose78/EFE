#include <stdio.h>
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON_GPIO 13   // GPIO number for the button
#define LED_GPIO 2      // GPIO number for the LED
#define AUTO_OFF_THRESHOLD  10000 //auto off threshold in ms
TickType_t led_on_tick;
static SemaphoreHandle_t led_mutex;

static QueueHandle_t gpio_evt_queue;
static bool led_state = 0;
// ISR Handler to handle the interrupt
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    // Send GPIO event to queue
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Task to handle the GPIO events
void gpio_task_handler(void* arg) {
    uint32_t io_num;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            xSemaphoreTake(led_mutex, portMAX_DELAY);  // Take the mutex

            //ESP_LOGI("GPIO", "GPIO event received, GPIO number: %d", io_num);  // Log when event is received
            led_state = !led_state;
            gpio_set_level(LED_GPIO, led_state);
            //Save the led on time tick
            if (led_state)
                led_on_tick = xTaskGetTickCount();
            xSemaphoreGive(led_mutex);  // Give the mutex
        }

        //give other task a chance to execute
        vTaskDelay(100 / portTICK_PERIOD_MS);  // Sleep for 1 second
    }
}

// Task to handle the auto off
void auto_off_handler(void* arg) {
    // Store the last wake time
    TickType_t xLastWakeTime;
    // Initialize xLastWakeTime to the current tick count
    xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        if (led_state) {
            xSemaphoreTake(led_mutex, portMAX_DELAY);  // Take the mutex

            uint32_t delta_time_ms = (xTaskGetTickCount() - led_on_tick) * (1000 / configTICK_RATE_HZ);

            if (AUTO_OFF_THRESHOLD < delta_time_ms) {
                ESP_LOGI("AUTO_OFF", "%d exceed threshold %d, switching off", delta_time_ms, AUTO_OFF_THRESHOLD);
                led_state = !led_state;
                gpio_set_level(LED_GPIO, led_state);
            }
            xSemaphoreGive(led_mutex);  // Give the mutex
        }

        // Delay for 1000ms (portTICK_PERIOD_MS gives tick duration in ms)
        vTaskDelayUntil(&xLastWakeTime, 1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    esp_err_t ret;

    // Initialize the mutex
    led_mutex = xSemaphoreCreateMutex();

    // Initialize the GPIO for LED (output)
    esp_rom_gpio_pad_select_gpio(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    // Initialize the GPIO for Button (input)
    esp_rom_gpio_pad_select_gpio(BUTTON_GPIO);
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_GPIO, GPIO_PULLUP_ONLY); // Enable internal pull-up
    gpio_set_intr_type(BUTTON_GPIO, GPIO_INTR_POSEDGE);

    // Create a queue to handle GPIO events
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    if (gpio_evt_queue == NULL) {
        ESP_LOGE("APP_MAIN", "Failed to create event queue!");
        return;
    }

    // Install ISR service
    ret = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);
    if (ret != ESP_OK) {
        ESP_LOGE("APP_MAIN", "isr service installation failed");
        return;
    }
    // Add GPIO interrupt handler for button press (falling edge)
    ret = gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, (void*) BUTTON_GPIO);
    if (ret != ESP_OK) {
        ESP_LOGE("APP_MAIN", "gpio_isr_handler_add failed");
        return;
    }

    // Create a task to handle the interrupt events
    xTaskCreate(gpio_task_handler, "gpio_task_handler", 2048, NULL, 9, NULL);

    // Create a task to handle the interrupt events
    xTaskCreate(auto_off_handler, "auto_off_task_handler", 2048, NULL, 10, NULL);

}
