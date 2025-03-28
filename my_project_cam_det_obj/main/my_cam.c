
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "my_cam.h"

static const char *TAG = "my_cam";
#define BOARD_ESP32S3_WROOM 1
#define CAM_SLEEP_TIME 1000000
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

    .xclk_freq_hz = 20000000,         //continous cam_hal: EV-VSYNC-OVF
//    .xclk_freq_hz = 10000000,       // intermettent cam_hal: EV-VSYNC-OVF
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
//    .pixel_format = PIXFORMAT_RGB565,
    .pixel_format = PIXFORMAT_JPEG,
//    .frame_size = FRAMESIZE_HQVGA,
    .frame_size = FRAMESIZE_VGA,
    .jpeg_quality = 12,
    .fb_count = 2,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
//    .grab_mode = CAMERA_GRAB_LATEST,
};

SemaphoreHandle_t cam_mutex = NULL; // Mutex to protect cam access
static int64_t last_capture_time; //access protect
static int cam_state; //access protect

#if 0
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
#endif

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
    //dump_cam_config();
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    // Get the sensor interface
    sensor_t *s = esp_camera_sensor_get();
    //https://randomnerdtutorials.com/esp32-cam-ov2640-camera-settings/
    s->set_brightness(s, 2);     // -2 to 2
    s->set_contrast(s, 2);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_special_effect(s, 2); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    s->set_ae_level(s, 0);       // -2 to 2
    s->set_aec_value(s, 600);    // 0 to 1200
    s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    s->set_agc_gain(s, 1);       // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)6);  // 0 to 6
    s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    s->set_vflip(s, 0);          // 0 = disable , 1 = enable
    s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

    cam_state = 1;
    return ESP_OK;
}

void create_cam_mut()
{
    cam_mutex = xSemaphoreCreateMutex();
    if (cam_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }
}

//should be accessed from protected sections
static void camera_power_down(void) {
    if (cam_state) {
        gpio_set_level(CAM_PIN_PWDN, 0); // Low level to power down (adjust as needed)
        //ESP_LOGI(TAG, "Camera powered down");
        cam_state = 0;
    }
}

//should be accessed from protected sections
static void camera_power_up(void) {
    if (!cam_state) {
        gpio_set_level(CAM_PIN_PWDN, 1); // High level to power up (adjust as needed)
        //ESP_LOGI(TAG, "Camera powered up");
        cam_state = 1;
    }
    last_capture_time = esp_timer_get_time();
}

void update_cam_stats(void) {
    if (xSemaphoreTake(cam_mutex, portMAX_DELAY) == pdTRUE) {
        camera_power_up();
        xSemaphoreGive(cam_mutex);
    }
}

/* 
 * Come in every 5000ms, check if time tick is > 10 sec, yes, de-init the cam
 * on next jpeg stream req cam will be powered on.
 */
void cam_task(void *arg) {

    esp_timer_handle_t timer_handle;
    esp_timer_create_args_t timer_args = {
        .callback = NULL, // No callback in this example
        .name = "time_compare_timer"
    };

    esp_timer_create(&timer_args, &timer_handle);

    last_capture_time = esp_timer_get_time(); //Initialize it.
    ESP_LOGI(TAG, "Started on CPU%d", xPortGetCoreID());
    while (1) {
        int64_t curr_time = esp_timer_get_time();
        if (xSemaphoreTake(cam_mutex, portMAX_DELAY) == pdTRUE) {
            int64_t elapsed_time = curr_time - last_capture_time;
            if (elapsed_time > CAM_SLEEP_TIME) {
                //more than 10 secs have passed!!
                camera_power_down();
            }
            // Release the mutex
            xSemaphoreGive(cam_mutex);
        }
 
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
