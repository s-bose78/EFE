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
#include "esp_pm.h"
//increase Component config->LWIP->TCP
//Default TCP rto time 1500
//Default Send Buffer Size 10240
//Default Receive Buffer Size 10240


static const char *TAG = "my_app_main";

void cpu_spped(void) {
    // Configure the CPU frequency to 240 MHz (for ESP32-S3)
    esp_pm_config_t pm_config = {
    .max_freq_mhz = 240, // Max CPU frequency (240 MHz)
    .min_freq_mhz = 240, // Min CPU frequency (80 MHz)
    .light_sleep_enable = 0 // Enable light sleep mode (optional)
    };
    
   // Initialize the power management
    int err = esp_pm_configure(&pm_config);
    if (err == ESP_OK) {
    ESP_LOGI("CPU", "CPU frequency range set to 80 MHz - 240 MHz");
    } else {
    ESP_LOGE("CPU", "Error setting CPU frequency err: %d", err);
    }
}

#define BLOCK_INIT_DELAY 500
void app_main(void)
{    
    cpu_spped();
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
    vTaskDelay(pdMS_TO_TICKS(BLOCK_INIT_DELAY));
    /*=============================== */
    if (init_camera()) {
        ESP_LOGE(TAG, "===> Cam Init Failed");
        return;
    } else {
        ESP_LOGI(TAG, "===> Cam Init Done");
    }
    vTaskDelay(pdMS_TO_TICKS(BLOCK_INIT_DELAY));
    /*=============================== */
    /* Init the wifi */
    wifi_base_init();
    wifi_init_sta();
    ESP_ERROR_CHECK(wifi_connect_sta());
    ESP_LOGI(TAG, "===> Wifi base connect Done");
    vTaskDelay(pdMS_TO_TICKS(BLOCK_INIT_DELAY));
    /*=============================== */
    create_cam_mut();
    //task for switching off the cam sensor if not captures occuring for long time
    //10secs, poor man power management.
    xTaskCreatePinnedToCore(cam_task, "cam_task", 4096, NULL, 1, NULL,1);
    vTaskDelay(pdMS_TO_TICKS(BLOCK_INIT_DELAY));
    /*=============================== */
    if (start_webserver()) {
        ESP_LOGE(TAG, "===> start_webserver Failed");
        return;
    } else {
        ESP_LOGI(TAG, "===> start_webserver Done");
    }

    //Init is done, no more traces other than error or warning.
    // Set the default log level to warning (or error, or none)
    esp_log_level_set("*", ESP_LOG_WARN); // or ESP_LOG_ERROR, or ESP_LOG_NONE

    // Set the log level for the HTTP module to error
    esp_log_level_set("HTTP", ESP_LOG_ERROR);

    // Set the log level for the esp-x509 module to none.
    esp_log_level_set("esp-x509-crt-bundle", ESP_LOG_NONE);

    vTaskDelay(pdMS_TO_TICKS(BLOCK_INIT_DELAY));
}
