#ifndef my_connect_h
#define my_connect_h

#include "esp_err.h"

void wifi_base_init(void);

void wifi_init_sta(void);
esp_err_t wifi_connect_sta(void);

void wifi_disconnect(void);
#endif