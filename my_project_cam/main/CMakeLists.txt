idf_component_register(SRCS "my_cam.c" "my_project_cam.c" "my_connect.c" "my_server.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_event esp_psram esp_wifi nvs_flash driver esp_http_server driver freertos)
