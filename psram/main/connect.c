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

static const char *TAG = "wifi connect";
static int s_retry_num = 0;
esp_netif_t *esp_netif_sta;

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
                ESP_LOGI(TAG,"connect to the AP fail");   
            }
        } else if (event_id == WIFI_EVENT_AP_START) {
            ESP_LOGI(TAG, "AP started");
        } else if (event_id == WIFI_EVENT_AP_STOP) {
            ESP_LOGI(TAG, "AP stopped");
        }   
    }
    if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            s_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

void wifi_base_init(void)
{
    ESP_LOGI(TAG, "=== %s",__func__);
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

void wifi_base_config(void)
{
    ESP_LOGI(TAG, "=== %s",__func__);
    s_retry_num = 0;
    s_wifi_event_group = xEventGroupCreate();

    esp_netif_sta = esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    wifi_config_t wifi_config_sta= {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta) );
    ESP_LOGI(TAG, "%s finished.", __func__);
}

esp_err_t wifi_connect_sta(void)
{
    ESP_LOGI(TAG, "=== %s",__func__);
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

/********************************* Web server **************************/
/*httpd server handle */
httpd_handle_t server = NULL;
esp_err_t favicon_handler(httpd_req_t *req) {
    // Send a 204 No Content response for the favicon request
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Favicon Not Found");

    return ESP_OK;
}

/* URI handler structure for /favicon.ico */
httpd_uri_t favicon_uri = {
    .uri      = "/",     // The URL path for the favicon
    .method   = HTTP_GET,           // HTTP GET method
    .handler  = favicon_handler,    // Handler function for the favicon
    .user_ctx = NULL                // No user context for the handler
};

esp_err_t help_handler(httpd_req_t *req) {
    // Simple HTML content for the help page
    const char *help_html = 
        "<html>"
        "<head><title>Help</title></head>"
        "<body>"
        "<h1>Welcome to the Help Page</h1>"
        "<p>For more details, consult the documentation.</p>"
        "</body>"
        "</html>";

    // Send the HTML response
    httpd_resp_send(req, help_html, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

/* URI handler structure for GET /help */
httpd_uri_t help_uri = {
    .uri      = "/help",                // The URL path for the help endpoint
    .method   = HTTP_GET,               // HTTP GET method
    .handler  = help_handler,           // Handler function for this endpoint
    .user_ctx = NULL                    // No user context for the handler
};

/* Function for starting the webserver */
httpd_handle_t start_webserver(void)
{
    ESP_LOGI(TAG, "=== %s",__func__);
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &help_uri);
        httpd_register_uri_handler(server, &favicon_uri);

    }

    /* If server failed to start, handle will be NULL */
    return server;
}

/* Function for stopping the webserver */
void stop_webserver(httpd_handle_t server)
{
    ESP_LOGI(TAG, "=== %s",__func__);
    if (server) {
        /* Stop the httpd server */
        httpd_stop(server);
    }
}

