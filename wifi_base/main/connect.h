#ifndef connect_h
#define connect_h

#include "esp_err.h"

#define CONFIG_WIFI_BASE_STA 1
#define CONFIG_WIFI_BASE_AP  0

void wifi_base_init(void);

#if CONFIG_WIFI_BASE_STA
void wifi_init_sta(void);
esp_err_t wifi_connect_sta(void);
#endif

#if CONFIG_WIFI_BASE_AP
void wifi_init_softap(void);
esp_err_t wifi_connect_ap(void);
#endif

void wifi_disconnect(void);
#endif