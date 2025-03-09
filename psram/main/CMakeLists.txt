idf_component_register(SRCS "web_srv_main.c" "connect.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_wifi esp_psram esp_partition spi_flash nvs_flash esp_http_server)
