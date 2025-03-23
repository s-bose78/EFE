#ifndef connect_h
#define connect_h

#include "esp_err.h"

#define CONFIG_WIFI_BASE_STA 1

void wifi_base_init(void);
void wifi_init_sta(void);
esp_err_t wifi_connect_sta(void);


void wifi_disconnect(void);
#endif