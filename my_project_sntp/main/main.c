/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <esp_chip_info.h>

#include "esp_flash.h"
#include "esp_netif.h"

#include "connect.h"
#include "sntp_l.h"

void nvs_init(void);
void esp_print_chip_info(void);


static const char *TAG = "main";

void task1(void *pvParameter) {
    while (1) {
        // Get time from SNTP server
        obtain_time();
        vTaskDelay(pdMS_TO_TICKS(10000));  // Wait for 2 seconds (2000 ms)
    }
}

void app_main(void)
{
    ESP_LOGI(TAG,"Hello world!");
    nvs_init();

    wifi_base_init();
    wifi_init_sta();    
    ESP_ERROR_CHECK_WITHOUT_ABORT(wifi_connect_sta());

    init_sntp();
    // California Time Zone
    set_time_zone("PST8PDT,M3.2.0/2,M11.1.0/2");

    xTaskCreate(task1, "Task 1", 4096, NULL, 5, NULL);

    // Main loop can also have code here if needed.
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Wait for 1 second (1000 ms)
    }

#if 0
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
#endif
}

void nvs_init(void)
{
  //Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}

/* Print chip information */
void esp_print_chip_info()
{
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
            (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
            (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
}
