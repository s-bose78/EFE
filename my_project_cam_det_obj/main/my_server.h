#pragma once
#include "esp_err.h"

void capture_task(void *arg);
esp_err_t start_webserver(void);