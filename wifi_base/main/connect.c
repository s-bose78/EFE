#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_netif.h"
#include "connect.h"
#include "driver/gpio.h"
#include <esp_http_server.h>

#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi_connect";
static int s_retry_num = 0;
esp_netif_t *esp_netif_sta;
esp_netif_t *esp_netif_ap;

#if CONFIG_WIFI_BASE_AP
esp_err_t start_webserver(void);
#endif
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG, "retry to connect to the AP");
            } else {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
            ESP_LOGE(TAG,"connect to the AP fail");
        } else if (event_id == WIFI_EVENT_AP_START) {
            ESP_LOGI(TAG, "AP started");
#if CONFIG_WIFI_BASE_AP
            start_webserver();
#endif
        } else if (event_id == WIFI_EVENT_AP_STOP) {
            ESP_LOGE(TAG, "AP stopped");
        }   
    }
    if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            s_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

void wifi_base_init(void)
{
    /* Init the tcp/ip stack */
    ESP_ERROR_CHECK(esp_netif_init());

    /* create a default event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
}

#if CONFIG_WIFI_BASE_STA
wifi_config_t wifi_config_sta= {
    .sta = {
        .ssid = EXAMPLE_ESP_WIFI_SSID,
        .password = EXAMPLE_ESP_WIFI_PASS,
        .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
        .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
    },
};

void wifi_init_sta(void)
{
    s_retry_num = 0;
    s_wifi_event_group = xEventGroupCreate();

    esp_netif_sta = esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta) );
    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

esp_err_t wifi_connect_sta(void)
{
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    ESP_LOGI(TAG, "============");
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
                ESP_LOGI(TAG, "connected to SSID:%s password:%s mode: station",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        ESP_LOGI(TAG, "============");
        return ESP_OK;   
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s mode: station",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    ESP_LOGI(TAG, "============");
    return ESP_FAIL;
}
#endif

#if CONFIG_WIFI_BASE_AP

// HTML content for the captive portal
const char *html_content = "<html><body><h1>Welcome to the Captive Portal!</h1></body></html>";

// HTTP GET handler to serve the captive portal page
esp_err_t captive_portal_handler(httpd_req_t *req) {
    httpd_resp_send(req, html_content, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Start the HTTP server
esp_err_t start_webserver() {
    httpd_handle_t server = NULL;
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    // Define URI handler for the captive portal
    httpd_uri_t captive_uri = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = captive_portal_handler,
        .user_ctx = NULL
    };

    if (httpd_start(&server, &config) == ESP_OK) {
        // Register the handler for the root URI
        httpd_register_uri_handler(server, &captive_uri);
        return ESP_OK;
    }
    return ESP_FAIL;
}

void wifi_init_softap(void)
{
    s_retry_num = 0;
    s_wifi_event_group = xEventGroupCreate();

    esp_netif_ap = esp_netif_create_default_wifi_ap();
    wifi_config_t wifi_config_ap= {
        .ap = {
            .ssid = "ESP32_AP",
            .ssid_len = strlen("ESP32_AP"),
            .password = "password",
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .max_connection = 5,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap) );
    ESP_LOGI(TAG, "wifi_init_ap finished.");
}

esp_err_t wifi_connect_ap(void)
{
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT | WIFI_EVENT_AP_START,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);
    ESP_LOGI(TAG, "============");
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
    * happened. */
    if (bits & (WIFI_CONNECTED_BIT| WIFI_EVENT_AP_START)) {
        ESP_LOGI(TAG, "connected to SSID:%s password:%s mode: ap",
                EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        ESP_LOGI(TAG, "============");
        return ESP_OK;   
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s mode: ap",
                EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    ESP_LOGI(TAG, "============");
    return ESP_FAIL;
}
#endif

void wifi_disconnect(void)
{
    s_retry_num = EXAMPLE_ESP_MAXIMUM_RETRY;
    esp_wifi_disconnect();
    esp_wifi_stop();
}