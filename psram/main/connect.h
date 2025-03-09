#ifndef CONNECT_H_
#define CONNECT_H_

#include "esp_err.h"
#include <esp_http_server.h>

void wifi_base_init(void);
void wifi_base_config(void);
esp_err_t wifi_connect_sta(void);

httpd_handle_t start_webserver(void);
void stop_webserver(httpd_handle_t server);
#endif