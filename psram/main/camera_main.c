#include <esp_system.h>
#include <nvs_flash.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "esp_camera.h"
#include "camera_pins.h"


static const char *TAG = "esp32-cam";


esp_err_t cam_init(void)
{

    return ESP_OK;
}