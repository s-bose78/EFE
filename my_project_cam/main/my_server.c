#include <stdio.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_camera.h"
#include "driver/gpio.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "my_cam.h"
#include "my_server.h"

static const char *TAG = "my_server";

static camera_fb_t *last_frame = NULL; // Pointer to store the last captured frame
static SemaphoreHandle_t frame_mutex = NULL; // Mutex to protect frame access
void capture_task(void *arg) {

    while (1) {
        // Capture a frame from the camera
        camera_fb_t *pic = esp_camera_fb_get();
        //ESP_LOGI(TAG, "Cam capture");
        if (pic) {
            // Lock the mutex to protect shared frame
            if (xSemaphoreTake(frame_mutex, portMAX_DELAY) == pdTRUE) {
                // If we already have a previous frame, free it
                if (last_frame) {
                    esp_camera_fb_return(last_frame);
                }

                // Store the new frame in the shared buffer
                last_frame = pic;

                // Release the mutex
                xSemaphoreGive(frame_mutex);
            }
        } else {
            ESP_LOGE(TAG, "Camera capture failed");
        }

        // Adjust the delay for frame capture rate
        vTaskDelay(pdMS_TO_TICKS(200)); // Capture a new frame every 250ms
    }
}

void create_mut()
{
    frame_mutex = xSemaphoreCreateMutex();
    if (frame_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }
    //test take and give once.
    if (xSemaphoreTake(frame_mutex, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "Semaphore taken successfully");
        // Critical section...
        xSemaphoreGive(frame_mutex);
    } else {
        ESP_LOGE(TAG, "Failed to take semaphore");
    }
}

esp_err_t capture_image_handler(httpd_req_t *req) {
    // Lock the mutex to ensure thread safety when accessing the camera
    if (xSemaphoreTake(frame_mutex, portMAX_DELAY) == pdTRUE) {

        // Capture the image from the camera
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            httpd_resp_send_500(req);  // Send 500 if capture fails
            return ESP_FAIL;
        }

        // Set the response type as "image/jpeg"
        httpd_resp_set_type(req, "image/jpeg");

        // Send the captured image (JPEG) as a response
        httpd_resp_send(req, (const char*)fb->buf, fb->len);

        // Return the framebuffer back to the driver for reuse
        esp_camera_fb_return(fb);

        // Release the mutex to allow other tasks to access the frame buffer
        xSemaphoreGive(frame_mutex);
    }

    return ESP_OK;
}

// Boundary for the multipart response
const char *boundary = "----ESP32_BOUNDARY";

esp_err_t index_handler(httpd_req_t *req) {
    const char *html_content =
    "<html>"
    "<head>"
    "<title>ESP32 Camera Web Server</title>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<style>"
    "  #countdown { font-size: 30px; font-weight: bold; }"
    "</style>"
    "</head>"
    "<body>"
    "<h1>ESP32S3 Camera Web Server</h1>"
    "<img id='camera' src='/capture' alt='ESP32 Camera Image' width='420' height='340'/>"
    "<br><br>"
    "<button onclick='captureImage()'>Capture Image</button>"
    "<br><br>"
    "<div id='countdown'></div>"
    "<script>"
    "function refreshImage() {"
    "    var img = document.getElementById('camera');"
    "    var timestamp = new Date().getTime();"
    "    img.src = '/capture?' + timestamp;"
    "}"
    "setInterval(refreshImage, 300);"
    "function captureImage() {"
    "    document.querySelector('button').disabled = true;"
    "    document.getElementById('countdown').innerHTML = 'Please hold steady...';"
    "    var countdown = 3;"
    "    var countdownInterval = setInterval(function() {"
    "        if (countdown > 0) {"
    "            document.getElementById('countdown').innerHTML = countdown;"
    "            countdown--;"
    "        } else {"
    "            clearInterval(countdownInterval);"
    "            fetch('/capture_image')"
    "                .then(response => response.blob())"
    "                .then(blob => {"
    "                    var link = document.createElement('a');"
    "                    link.href = URL.createObjectURL(blob);"
    "                    link.download = 'esp32_capture.jpg';"
    "                    link.click();"
    "                    document.querySelector('button').disabled = false;"
    "                    document.getElementById('countdown').innerHTML = '';"
    "                });"
    "        }"
    "    }, 1000);"
    "}"
    "</script>"
    "</body>"
    "</html>";

    // Send the HTML content as a response
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_content, strlen(html_content));
    return ESP_OK;
}


#if 0
esp_err_t index_handler(httpd_req_t *req) {
    const char *html_content =
    "<html>"
    "<head>"
    "<title>ESP32 Camera Web Server</title>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"  // Makes it mobile-friendly
    "</head>"
    "<body>"
    "<h1>ESP32S3 Camera Web Server</h1>"
    "<img id='camera' src='/capture' alt='ESP32 Camera Image' width='420' height='340'/>"
    "<script>"
    "function refreshImage() {"
    "    var img = document.getElementById('camera');"
    "    var timestamp = new Date().getTime();"
    "    img.src = '/capture?' + timestamp;"
    ""
    "}"
    "setInterval(refreshImage, 300);"
    "</script>"
    "</body>"
    "</html>";

    // Send the HTML content as response
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_content, strlen(html_content));
    return ESP_OK;
}
#endif
esp_err_t jpeg_stream_handler(httpd_req_t *req) {
    // Lock the mutex to ensure thread safety when accessing the camera
    if (xSemaphoreTake(frame_mutex, portMAX_DELAY) == pdTRUE) {
        // Capture a single frame
        camera_fb_t *pic = esp_camera_fb_get();  // Capture a single frame

        if (pic) {
            // Set the response type to JPEG for single image capture
            httpd_resp_set_type(req, "image/jpeg");
            httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
            httpd_resp_set_hdr(req, "Pragma", "no-cache");
            
            // Send the image in chunks
            size_t chunk_size = 4096;  // Send data in chunks of 512 bytes
            size_t offset = 0;
            esp_err_t err = ESP_OK;
            while (offset < pic->len) {
                size_t chunk_len = (pic->len - offset < chunk_size) ? (pic->len - offset) : chunk_size;
                err = httpd_resp_send_chunk(req, (const char *)(pic->buf + offset), chunk_len);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to send image chunk, offset: %d, err: %d", offset, err);
                    break;
                }
                offset += chunk_len;
            }

            // If sending the chunks was successful
            if (err == ESP_OK) {
                err = httpd_resp_send_chunk(req, NULL, 0);  // Sending an empty chunk signals the end of the response
            } else {
                ESP_LOGE(TAG, "Failed to send image in chunks");
            }

            // Return the frame buffer back to the driver
            esp_camera_fb_return(pic);
        } else {
            ESP_LOGE(TAG, "Failed to capture frame");
            httpd_resp_send_500(req);  // Send HTTP 500 error if frame capture failed
        }

        // Release the mutex to allow other tasks to access the frame buffer
        xSemaphoreGive(frame_mutex);
    }

    return ESP_OK;
}

// HTTP server configuration
httpd_uri_t index_uri = {
    .uri = "/",  // Root URI for the main page
    .method = HTTP_GET,
    .handler = index_handler,  // The handler function to serve the HTML page
    .user_ctx = NULL
};

// HTTP server configuration
httpd_uri_t jpeg_stream_uri = {
    .uri = "/capture",  // URI for the endpoint
    .method = HTTP_GET,  // HTTP GET request
    .handler = jpeg_stream_handler,  // The handler function to capture the image
    .user_ctx = NULL
};

// Register the HTTP handler for capturing an image ("/capture_image")
httpd_uri_t capture_image_uri = {
    .uri = "/capture_image",
    .method = HTTP_GET,
    .handler = capture_image_handler,
    .user_ctx = NULL
};

esp_err_t start_webserver() {
    httpd_handle_t server = NULL;

    // Configure the HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    // Start the server
    if (httpd_start(&server, &config) == ESP_OK) {
        // Register URI handlers
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &jpeg_stream_uri);
        httpd_register_uri_handler(server, &capture_image_uri);
    } else {
        ESP_LOGI(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }
    return ESP_OK;
}