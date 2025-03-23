idf_component_register(SRCS "sntp_l.c" "main.c" "connect.c"
                       INCLUDE_DIRS ""
                       REQUIRES spi_flash esp_netif esp_netif esp_wifi nvs_flash driver esp_http_server esp_http_client)

