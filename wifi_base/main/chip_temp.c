#include "driver/temperature_sensor.h"

temperature_sensor_handle_t temp_handle = NULL;
temperature_sensor_config_t temp_sensor = {
    .range_min = 20,
    .range_max = 50,
};

esp_err_t temp_sensor_init()
{
    return temperature_sensor_install(&temp_sensor, &temp_handle);
}

esp_err_t temp_sensor_enable()
{
    return temperature_sensor_enable(temp_handle);
}

esp_err_t temp_sensor_disable()
{
    return temperature_sensor_disable(temp_handle);
}

float temp_sensor_get_c()
{
    float celci;

    temperature_sensor_get_celsius(temp_handle, &celci);

    return celci;
}