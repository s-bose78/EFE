#pragma once

#include "esp_err.h"

esp_err_t temp_sensor_init(void);
esp_err_t temp_sensor_enable(void);
esp_err_t temp_sensor_disable(void);
float temp_sensor_get_c(void);