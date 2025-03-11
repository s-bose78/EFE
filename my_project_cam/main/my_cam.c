
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "my_cam.h"

static const char *TAG = "my_cam";
#define BOARD_ESP32S3_WROOM 1

// ESP32S3 (WROOM) PIN Map
#ifdef BOARD_ESP32S3_WROOM
#define CAM_PIN_PWDN 38
#define CAM_PIN_RESET -1   //software reset will be performed
#define CAM_PIN_VSYNC 6
#define CAM_PIN_HREF 7
#define CAM_PIN_PCLK 13
#define CAM_PIN_XCLK 15
#define CAM_PIN_SIOD 4
#define CAM_PIN_SIOC 5
#define CAM_PIN_D0 11
#define CAM_PIN_D1 9
#define CAM_PIN_D2 8
#define CAM_PIN_D3 10
#define CAM_PIN_D4 12
#define CAM_PIN_D5 18
#define CAM_PIN_D6 17
#define CAM_PIN_D7 16
#endif

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

//    .xclk_freq_hz = 20000000,         //continous cam_hal: EV-VSYNC-OVF
    .xclk_freq_hz = 10000000,       // intermettent cam_hal: EV-VSYNC-OVF
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
//    .pixel_format = PIXFORMAT_RGB565,
    .pixel_format = PIXFORMAT_JPEG,
//    .frame_size = FRAMESIZE_QVGA,
    .frame_size = FRAMESIZE_240X240,
    .jpeg_quality = 50,
    .fb_count = 20,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
//    .grab_mode = CAMERA_GRAB_LATEST,
};

static void dump_cam_config()
{
    ESP_LOGI(TAG, "camera_config\n" 
        "pin_pwdn = %d pin_reset = %d pin_xclk = %d\n"
        "pin_sccb_sda = %d pin_sccb_scl = %d pin_d7 = %d\n"
        "pin_d6 = %d pin_d5 = %d pin_d4 = %d\n"
        "pin_d3 = %d pin_d2 = %d pin_d1 = %d\n"
        "pin_d0 = %d pin_vsync = %d pin_href = %d\n"
        "pin_pclk = %d xclk_freq_hz = %d ledc_timer = %d\n"
        "ledc_channel = %d pixel_format = %d frame_size = %d\n"
        "jpeg_quality = %d fb_count = %d fb_location = %d\n"
        "grab_mode = %d\n",
        camera_config.pin_pwdn, camera_config.pin_reset, camera_config.pin_xclk,
        camera_config.pin_sccb_sda, camera_config.pin_sccb_scl, camera_config.pin_d7,
        camera_config.pin_d6, camera_config.pin_d5, camera_config.pin_d4,
        camera_config.pin_d3, camera_config.pin_d2, camera_config.pin_d1,
        camera_config.pin_d0, camera_config.pin_vsync, camera_config.pin_href,
        camera_config.pin_pclk, camera_config.xclk_freq_hz, camera_config.ledc_timer,
        camera_config.ledc_channel, camera_config.pixel_format, camera_config.frame_size,
        camera_config.jpeg_quality, camera_config.fb_count, camera_config.fb_location,
        camera_config.grab_mode);
}

void take_picture(camera_fb_t *pic)
{
    ESP_LOGI(TAG, "Taking picture...");
    if (pic)
        pic = esp_camera_fb_get();
}

void return_fb(camera_fb_t *pic)
{
    if (pic)
        esp_camera_fb_return(pic);
}

esp_err_t init_camera(void)
{
    dump_cam_config();
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

