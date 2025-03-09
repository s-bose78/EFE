idf_component_register(SRCS "my_project_ota.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_psram esp_partition spi_flash nvs_flash sdmmc fatfs app_update)
