#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_psram.h"
#include "sdkconfig.h"
#include "esp_cpu.h"
#include "esp_err.h"

#include "my_connect.h"
#include "my_cam.h"
#include "my_server.h"

static const char *TAG = "my_app_main";
void app_main(void)
{
    /*=============================== */
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    /*=============================== */
    /* Check PSRAM initialized and size */
    ESP_LOGI(TAG, "===> Check PSRAM Size=%d Init=%d", esp_psram_get_size(), esp_psram_is_initialized());
    vTaskDelay(pdMS_TO_TICKS(2000));  // Delay 1 second (1000 ms)

    /*=============================== */
    //ESP_ERROR_CHECK(init_camera());
    if (init_camera()) {
        ESP_LOGE(TAG, "===> Cam Init Failed");
        return;
    } else {
        ESP_LOGI(TAG, "===> Cam Init Done");
    }
    vTaskDelay(pdMS_TO_TICKS(2000));  // Delay 1 second (1000 ms)
    /*=============================== */
    /* Init the wifi */
    wifi_base_init();
    wifi_init_sta();
    ESP_ERROR_CHECK(wifi_connect_sta());
    ESP_LOGI(TAG, "===> Wifi base connect Done");
    vTaskDelay(pdMS_TO_TICKS(2000));  // Delay 1 second (1000 ms)
    /*=============================== */

    create_mut();
    xTaskCreate(capture_task, "capture_task", 4096, NULL, 1, NULL);

    if (start_webserver()) {
        ESP_LOGE(TAG, "===> start_webserver Failed");
        return;
    } else {
        ESP_LOGI(TAG, "===> start_webserver Done");
    }

}
