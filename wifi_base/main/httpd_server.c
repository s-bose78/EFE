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


static const char *TAG = "HTTPD_SERVER";
#define MIN(a,b ) ((a <= b)? a : b)
#define LED_GPIO_PIN GPIO_NUM_2 // Define the GPIO pin connected to the LED
/* Send a simple response */
const char get_resp[] = "URI GET Response";
/* Send a simple response */
const char post_resp[] = "URI POST Response\n";

int led_state = 0;
char on_resp[] = "<!DOCTYPE html><html><head><style type=\"text/css\">html { font-family: 'Roboto', sans-serif; background-color: #f4f7fc; margin: 0; text-align: center;}.container { max-width: 1000px; margin: 0 auto; padding: 20px;}.header { font-size: 2rem; color: #4c4c4c; margin-top: 20px;}.card { background-color: #ffffff; box-shadow: 0px 8px 16px rgba(0, 0, 0, 0.1); border-radius: 8px; padding: 20px; margin: 20px; text-align: center;}.card p { font-size: 1.2rem; color: #333;}.button { display: inline-block; background-color: #ff5c5c; border: none; border-radius: 4px; color: white; padding: 15px 40px; text-decoration: none; font-size: 18px; margin: 10px; cursor: pointer; transition: background-color 0.3s ease;}.button:hover { background-color: #e04a4a;}.button2 { background-color: #0066cc;}.button2:hover { background-color: #0055b3;}.fas { margin-bottom: 10px;}.footer { font-size: 0.9rem; color: #7a7a7a; position: absolute; bottom: 10px; width: 100%;}</style><title>ESP32 WEB SERVER</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><link rel=\"icon\" href=\"data:,\"><link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"><link rel=\"stylesheet\" type=\"text/css\"></head><body><div class=\"container\"><h2 class=\"header\">ESP32 WEB SERVER</h2><div class=\"card\"><p><i class=\"fas fa-lightbulb fa-3x\" style=\"color: #ff5c5c;\"></i><br><strong>GPIO2</strong></p><p>GPIO state: <strong>ON</strong></p><p><a href=\"/ledoff\"><button class=\"button button2\">Turn OFF</button></a></p></div></div><div class=\"footer\">ESP32 Powered Web Server</div></body></html>";
char off_resp[] = "<!DOCTYPE html><html><head><style type=\"text/css\">html { font-family: 'Roboto', sans-serif; background-color: #f4f7fc; margin: 0; text-align: center;}.container { max-width: 1000px; margin: 0 auto; padding: 20px;}.header { font-size: 2rem; color: #4c4c4c; margin-top: 20px;}.card { background-color: #ffffff; box-shadow: 0px 8px 16px rgba(0, 0, 0, 0.1); border-radius: 8px; padding: 20px; margin: 20px; text-align: center;}.card p { font-size: 1.2rem; color: #333;}.button { display: inline-block; background-color: #ff5c5c; border: none; border-radius: 4px; color: white; padding: 15px 40px; text-decoration: none; font-size: 18px; margin: 10px; cursor: pointer; transition: background-color 0.3s ease;}.button:hover { background-color: #e04a4a;}.button2 { background-color: #0066cc;}.button2:hover { background-color: #0055b3;}.fas { margin-bottom: 10px;}.footer { font-size: 0.9rem; color: #7a7a7a; position: absolute; bottom: 10px; width: 100%;}</style><title>ESP32 WEB SERVER</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><link rel=\"icon\" href=\"data:,\"><link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"><link rel=\"stylesheet\" type=\"text/css\"></head><body><div class=\"container\"><h2 class=\"header\">ESP32 WEB SERVER</h2><div class=\"card\"><p><i class=\"fas fa-lightbulb fa-3x\" style=\"color: #ff5c5c;\"></i><br><strong>GPIO2</strong></p><p>GPIO state: <strong>OFF</strong></p><p><a href=\"/ledon\"><button class=\"button\">Turn ON</button></a></p></div></div><div class=\"footer\">ESP32 Powered Web Server</div></body></html>";

// Dummy Function to read temperature and humidity
esp_err_t get_temperature_handler(httpd_req_t *req)
{
    float temp = 25.6; // Read temperature TBD put a call here to fetch from internet.
    float humidity = 50.2; // Read humidity TBD put a call here to fetch from internet.

    // JSON Response
    char response[1024];
    ESP_LOGI(TAG, "temperature_handler");
    //snprintf(response, sizeof(response), "{\"temperature\": %.2f, \"humidity\": %.2f}", temp, humidity);
    snprintf(response, sizeof(response),
    "<!DOCTYPE html><html><head><style type=\"text/css\">"
    "html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center; }"
    "h1 { color: #333; padding: 2vh; }"
    ".content { padding: 50px; }"
    ".card { background-color: #f4f4f4; box-shadow: 2px 2px 12px rgba(140,140,140,.5); padding: 20px; }"
    ".card p { font-size: 1.5rem; color: #555; }"
    "</style><title>ESP32 Sensor Data</title>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "</head><body>"
    "<h1>ESP32 Temperature and Humidity Sensor</h1>"
    "<div class=\"content\">"
    "<div class=\"card\">"
    "<p><strong>Temperature:</strong> %.2f &deg;C</p>"
    "<p><strong>Humidity:</strong> %.2f %%</p>"
    "</div>"
    "</div>"
    "</body></html>", temp, humidity);

    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

esp_err_t led_on_handler(httpd_req_t *req)
{
    set_gpio(LED_PIN, 1);
    led_state = 1;
    httpd_resp_send(req, on_resp, HTTPD_RESP_USE_STRLEN);  // Return OFF page
    return ESP_OK;
}

esp_err_t led_off_handler(httpd_req_t *req)
{
    set_gpio(LED_PIN, 0);
    led_state = 0;
    httpd_resp_send(req, off_resp, HTTPD_RESP_USE_STRLEN);  // Return OFF page
    return ESP_OK;
}

esp_err_t get_led_state_handler(httpd_req_t *req)
{
    const char* response_template = 
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<style>"
    "body { font-family: Arial, sans-serif; background-color: #f4f4f4; margin: 0; padding: 20px; text-align: center; }"
    "h1 { color: #4CAF50; }"
    ".button { display: inline-block; background-color: #4CAF50; color: white; padding: 16px 40px; text-decoration: none; font-size: 20px; border-radius: 5px; margin-top: 20px; }"
    ".status { font-size: 24px; font-weight: bold; margin-top: 30px; }"
    "a { text-decoration: none; }"
    "</style>"
    "<title>LED Control</title>"
    "</head>"
    "<body>"
    "<h1>ESP32 WEB SERVER</h1>"
    "<p class='status'>LED State: <span style='color: %s;'>%s</span></p>"
    "<a href='/ledon' class='button'>Turn LED ON</a><br><br>"
    "<a href='/ledoff' class='button' style='background-color: #f44336;'>Turn LED OFF</a>"
    "</body>"
    "</html>";

    const char *color = led_state ? "green" : "red";
    const char *state = led_state ? "ON" : "OFF";

    // Format the HTML response
    char response[1024];
    snprintf(response, sizeof(response), response_template, color, state);

    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);  // Return OFF page
    return ESP_OK;
}

esp_err_t help_handler(httpd_req_t *req) {
    // Simple HTML content for the help page
    const char *help_html = 
        "<html>"
        "<head><title>Help</title></head>"
        "<body>"
        "<h1>Welcome to the Help Page</h1>"
        "<ul>"
        "<li><strong>/get_temp</strong> - Get the current temperature</li>"
        "<li><strong>/get_led_state</strong> - Get the current LED state</li>"
        "<li><strong>/ledon</strong> - Turn the LED on</li>"
        "<li><strong>/ledoff</strong> - Turn the LED off</li>"
        "</ul>"
        "<p>For more details, consult the documentation.</p>"
        "</body>"
        "</html>";

    // Send the HTML response
    httpd_resp_send(req, help_html, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

/* URI handler structure for GET /help */
httpd_uri_t help_uri = {
    .uri      = "/",                // The URL path for the help endpoint
    .method   = HTTP_GET,               // HTTP GET method
    .handler  = help_handler,           // Handler function for this endpoint
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
        httpd_register_uri_handler(server, &help_uri);
        httpd_register_uri_handler(server, &temp_get);
        httpd_register_uri_handler(server, &get_led_state);
        httpd_register_uri_handler(server, &led_on);
        httpd_register_uri_handler(server, &led_off);
    }

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

