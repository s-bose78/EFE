idf_component_register(SRCS "main.c" "connect.c" "httpd_server.c" "http_client.c" "led_gpio.c" "chip_temp.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi nvs_flash driver esp_http_server esp_http_client)