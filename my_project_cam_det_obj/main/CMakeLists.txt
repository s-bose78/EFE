idf_component_register(SRCS "my_cam.c" "my_project_cam.c" "my_connect.c" "my_server.c" "my_http_client.c"
                    INCLUDE_DIRS "."
                    REQUIRES json mbedtls esp-tls esp_event esp_psram esp_wifi nvs_flash driver esp_http_server esp_http_client driver freertos esp_timer)
