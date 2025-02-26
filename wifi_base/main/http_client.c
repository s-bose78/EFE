#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_log.h"



// Replace with your own API Key from OpenWeatherMap
#define LATITUDE 49.2827
#define LONGITUDE -123.1207

#define TAG "CLIENT_APP"

// Fetch weather data
esp_err_t fetch_weather_data()
{
    // Prepare a buffer to store the response body
    char response[1024];
    int content_length;
    int bytes_read;

    // Construct the full URL with query parameters for OpenWeatherMap API
    char url[256];
    snprintf(url, sizeof(url), "http://api.open-meteo.com/v1/forecast?latitude=%f&longitude=%f&current_weather=true", LATITUDE, LONGITUDE); 
    ESP_LOGI(TAG, "url: %s", url);

//  esp_http_client_set_header(client, "User-Agent", "ESP32-Weather-App");
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,

    // .skip_cert_common_name_check = true,  // Skip SSL certificate validation
    // .transport_type = HTTP_TRANSPORT_OVER_TCP,  // Use HTTP instead of HTTPS
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_url(client, url);

    // Send the HTTP GET request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status Code: %d", status_code);
        content_length = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "content_length: %d", content_length);
        // Read the response body into the buffer
        bytes_read = esp_http_client_read_response(client, response, content_length);

        if (bytes_read > 0) {
            response[bytes_read] = '\0';  
            ESP_LOGI(TAG, "%d Weather data: %s", bytes_read, response);
        } else {
            ESP_LOGE(TAG, "Failed to read response body");
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

#define WHATSAPP_API_URL "https://api.callmebot.com/whatsapp.php"
#define PHONE_NUMBER "16043071081"
#define API_KEY "8027895"
#define MESSAGE "Hello from ESP32!"

// Define the url_encode function
char* url_encode(const char *str) {
    int len = strlen(str);
    char *encoded = malloc(3 * len + 1); // Allocate enough space for encoding
    int pos = 0;

    for (int i = 0; i < len; i++) {
        unsigned char c = str[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded[pos++] = c;
        } else {
            pos += sprintf(&encoded[pos], "%%%02X", c);
        }
    }
    encoded[pos] = '\0';
    return encoded;
}

void send_whatsapp_message(void)
{
    // Build the URL with parameters
    char url[512];
    char *encoded_message = url_encode(MESSAGE);
    snprintf(url, sizeof(url), "%s?phone=%s&apikey=%s&text=%s", 
             WHATSAPP_API_URL, PHONE_NUMBER, API_KEY, encoded_message);

    ESP_LOGI(TAG, "url: %s", url);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .cert_pem = NULL, 
        //.disable_ssl_hostname_check = true
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);


    esp_http_client_set_url(client, url);
    // Perform the GET request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("HTTP", "WhatsApp message sent successfully.");
    } else {
        ESP_LOGE("HTTP", "Failed to send WhatsApp message. Error: %s", esp_err_to_name(err));
    }

    // Cleanup
    esp_http_client_cleanup(client);
}

