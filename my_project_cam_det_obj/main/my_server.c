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

#include "my_http_client.h"

static const char *TAG = "my_server";

//post the captured image to http_client for analysis to google api
esp_err_t analyze_image_handler(httpd_req_t *req) {
    //ESP_LOGI(TAG, "==%s",__func__);
    //update camera stats
    update_cam_stats();

    // Capture the image from the camera
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        httpd_resp_send_500(req);  // Send 500 if capture fails
        return ESP_FAIL;
    }

    //Send the image for analyis.
    sendImageFrAnalysis((const uint8_t *)fb->buf, fb->len);

    if (resp_decoded) {
        // Send the description as plain text
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_send(req, resp_output, strlen(resp_output));
    }
    // Return the framebuffer back to the driver for reuse
    esp_camera_fb_return(fb);

    return ESP_OK;
}

//captive page
static const char *html_content =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title>ESP32S3 WROOM Camera</title>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "<style>"
    "body { font-family: sans-serif; margin: 0; padding: 20px; }"
    "h1 { text-align: center; }"
    "#camera-img { display: block; margin: 20px auto; max-width: 100%; height: auto; }"
    "#capture-btn { display: block; margin: 20px auto; padding: 10px 20px; font-size: 16px; cursor: pointer; }"
    "#countdown { text-align: center; font-size: 24px; font-weight: bold; margin-bottom: 10px; }"
    "#description-label { font-size: 18px; margin-top: 20px; }"
    "#description-box { font-size: 16px; padding: 10px; width: 80%; margin: 10px auto; border: 1px solid #ccc; white-space: pre-wrap; overflow-wrap: break-word; background-color: #f9f9f9; }"
    "#resume-btn { display: none; margin: 20px auto; padding: 10px 20px; font-size: 16px; cursor: pointer; }"
    "</style>"
    "</head>"
    "<body>"
    "<h1>ESP32 Camera Feed</h1>"
    "<img id=\"camera-img\" src=\"/capture\" alt=\"Camera Feed\">"
    "<button id=\"capture-btn\" onclick=\"captureAnalysis()\">Analyze Image</button>"
    "<div id=\"countdown\"></div>"
    "<div id=\"description-label\">Analysis Result:</div>"
    "<pre id=\"description-box\"></pre>"
    "<button id=\"resume-btn\" onclick=\"resumeCamera()\">Resume Camera Feed</button>"
    "<script>"
    "let cameraInterval;"
    "function refreshCamera() { document.getElementById('camera-img').src = '/capture?' + Date.now(); }"
    "function startCameraRefresh() { cameraInterval = setInterval(refreshCamera, 300); }"
    "function stopCameraRefresh() { clearInterval(cameraInterval); }"
    "startCameraRefresh();"
    "function captureAnalysis() {"
    "  document.getElementById('capture-btn').disabled = true;"
    "  document.getElementById('countdown').textContent = 'Analyzing...';"
    "  let timer = 3;"
    "  let interval = setInterval(() => {"
    "    if (timer > 0) { document.getElementById('countdown').textContent = timer--; }"
    "    else {"
    "      clearInterval(interval);"
    "      stopCameraRefresh();"
    "      fetch('/analyze_image')"
    "        .then(res => res.text())"
    "        .then(data => {"
    "          document.getElementById('description-box').textContent = data || 'Analysis failed.';"
    "          document.getElementById('capture-btn').disabled = false;"
    "          document.getElementById('countdown').textContent = '';"
    "          document.getElementById('resume-btn').style.display = 'block';" //Correct display
    "        });"
    "    }"
    "  }, 1000);"
    "}"
    "function resumeCamera() {"
    "  startCameraRefresh();"
    "  document.getElementById('resume-btn').style.display = 'none';" //Correct display
    "}"
    "</script>"
    "</body>"
    "</html>";

esp_err_t index_handler(httpd_req_t *req) {
    // Send the HTML content as a response
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_content, strlen(html_content));
    return ESP_OK;
}

esp_err_t jpeg_stream_handler(httpd_req_t *req) {
    //ESP_LOGI(TAG, "==%s",__func__);
    //update camera stats
    update_cam_stats();
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
                //ESP_LOGE(TAG, "Failed to send image chunk, offset: %d, err: %d", offset, err);
                break;
            }
            offset += chunk_len;
        }

        // If sending the chunks was successful
        if (err == ESP_OK) {
            err = httpd_resp_send_chunk(req, NULL, 0);  // Sending an empty chunk signals the end of the response
        //} else {
        //    ESP_LOGE(TAG, "Failed to send image in chunks");
        }

        // Return the frame buffer back to the driver
        esp_camera_fb_return(pic);
    } else {
        //ESP_LOGE(TAG, "Failed to capture frame");
        httpd_resp_send_500(req);  // Send HTTP 500 error if frame capture failed
    }

    return ESP_OK;
}

/* All Handlers here */
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

// Register the HTTP handler for analyzing the image
httpd_uri_t analyze_image_uri = {
    .uri = "/analyze_image",
    .method = HTTP_GET,
    .handler = analyze_image_handler,
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
        httpd_register_uri_handler(server, &analyze_image_uri);
    } else {
        ESP_LOGI(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }
    return ESP_OK;
}
