#ifndef my_cam_h
#define my_cam_h

#include "esp_camera.h"

void update_cam_stats(void);
esp_err_t init_camera(void);
void take_picture(camera_fb_t *pic);
void return_fb(camera_fb_t *pic);
void create_cam_mut();
void cam_task(void *arg);
#endif