/* Our URI handler function to be called during GET /uri request */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //Requires by memset
#include <esp_err.h>
#include <esp_http_server.h>
#include <rom/gpio.h>

#include <esp_netif_ip_addr.h>
#include "esp_log.h"
#include "led_gpio.h"
#include "server_html.h"
#include "chip_temp.h"

static const char *TAG = "HTTPD_SERVER";
#define MIN(a,b ) ((a <= b)? a : b)

int led_state = 0;

static void send_httpd_resp(httpd_req_t *req, const char *str)
{
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    httpd_resp_send(req, str, HTTPD_RESP_USE_STRLEN);
}

// Dummy Function to read temperature and humidity
esp_err_t get_temperature_handler(httpd_req_t *req)
{
    float temp_c = temp_sensor_get_c(); // Read temperature TBD put a call here to fetch from internet.

    memset(temp_html, 0, sizeof(temp_html));
    snprintf(temp_html, sizeof(temp_html), temp_html_body, temp_c);
    send_httpd_resp(req, temp_html);

    return ESP_OK;
}

esp_err_t led_on_handler(httpd_req_t *req)
{
    set_gpio(LED_PIN, 1);
    led_state = 1;
    send_httpd_resp(req, ledon_html);

    return ESP_OK;
}

esp_err_t led_off_handler(httpd_req_t *req)
{
    set_gpio(LED_PIN, 0);
    led_state = 0;
    send_httpd_resp(req, ledoff_html);

    return ESP_OK;
}

#define LED_STATE(a) ((a == 1)?"ON":"OFF")
esp_err_t get_led_state_handler(httpd_req_t *req)
{
    memset(temp_html, 0, sizeof(temp_html));
    snprintf(temp_html, sizeof(temp_html), led_state_html_body, LED_STATE(led_state));
    send_httpd_resp(req, temp_html);

    return ESP_OK;
}

esp_err_t home_handler(httpd_req_t *req) {

    send_httpd_resp(req, home_html);

    return ESP_OK;
}

/* URI handler structure for GET /help */
httpd_uri_t home_uri = {
    .uri      = "/",                // The URL path for the help endpoint
    .method   = HTTP_GET,               // HTTP GET method
    .handler  = home_handler,           // Handler function for this endpoint
    .user_ctx = NULL                    // No user context for the handler
};

/* URI handler structure for GET /temp */
httpd_uri_t temp_get = {
    .uri      = "/get_temp",
    .method   = HTTP_GET,
    .handler  = get_temperature_handler,
    .user_ctx = NULL
};

httpd_uri_t get_led_state = {
    .uri = "/get_led_state",            // URL path to get the LED state
    .method = HTTP_GET,             // HTTP GET method
    .handler = get_led_state_handler, // Handler function for this endpoint
    .user_ctx = NULL
};

/* URI handler structure for GET /uri */
httpd_uri_t led_on = {
    .uri      = "/ledon",
    .method   = HTTP_GET,
    .handler  = led_on_handler,
    .user_ctx = NULL
};

/* URI handler structure for GET /uri */
httpd_uri_t led_off = {
    .uri      = "/ledoff",
    .method   = HTTP_GET,
    .handler  = led_off_handler,
    .user_ctx = NULL
};

/* Function for starting the webserver */
httpd_handle_t start_webserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &home_uri);
        httpd_register_uri_handler(server, &temp_get);
        httpd_register_uri_handler(server, &get_led_state);
        httpd_register_uri_handler(server, &led_on);
        httpd_register_uri_handler(server, &led_off);
    }

    if(!server)
        ESP_LOGE(TAG,"Failed to start httpd server");

    /* If server failed to start, handle will be NULL */
    return server;
}

/* Function for stopping the webserver */
void stop_webserver(httpd_handle_t server)
{
    if (server) {
        /* Stop the httpd server */
        httpd_stop(server);
    }
}

