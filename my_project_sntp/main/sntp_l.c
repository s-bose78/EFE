#include <esp_log.h>

#include "esp_system.h"
#include "esp_netif_types.h"
#include "esp_netif_sntp.h"
#include "esp_netif.h"
#include "esp_sntp.h"

static const char *TAG = "SNTP";

// SNTP Server(s)
#define SNTP_SERVER1 "pool.ntp.org"
#define SNTP_SERVER2 "time.windows.com"
#define SNTP_SERVER3 "time.google.com"

void init_sntp(void) {
    // Initialize SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);  // Set SNTP mode to polling
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(3,
        ESP_SNTP_SERVER_LIST(SNTP_SERVER1, SNTP_SERVER2, SNTP_SERVER3));
    esp_netif_sntp_init(&config);    // Add as many NTP servers as needed

    // Wait until time is synchronized
    ESP_LOGI("SNTP", "Waiting for time sync");
    int attempts = 0;
    sntp_sync_status_t status;

    while (attempts < 10) {
        // Wait for 5 second and try again
        vTaskDelay(pdMS_TO_TICKS(5000));
        status = sntp_get_sync_status();
        ESP_LOGI(TAG, "SNTP attempts: %d status %d", attempts, status);
        if (status == SNTP_SYNC_STATUS_COMPLETED) {
            ESP_LOGI(TAG, "SNTP synchronization successful status %d", sntp_get_sync_status());
            return;
        }

        attempts++;
    }

    ESP_LOGW(TAG, "SNTP synchronization failed status %d", sntp_get_sync_status());
}

// Function to set the time zone
void set_time_zone(const char* timezone) {
    // Set the environment variable TZ to the desired time zone string
    setenv("TZ", timezone, 1);  // Example: "UTC-5" for EST, "CET-1CEST,M3.5.0,M10.5.0/3"
    tzset();  // Apply the time zone setting
}

void obtain_time(void) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI("SNTP", "Current local time: %04d-%02d-%02d %02d:%02d:%02d",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}


