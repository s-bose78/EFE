/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_system.h"


#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "connect.h"
#include "http_client.h"
#include "httpd_server.h"
#include "esp_netif.h"
#include <esp_netif_ip_addr.h>

#include "led_gpio.h"
#include "chip_temp.h"

static const char *TAG = "main";

static void disable_module_logging(void)
  {
  esp_log_level_set("wifi", ESP_LOG_NONE);
  esp_log_level_set("phy_init", ESP_LOG_NONE);
  esp_log_level_set("wifi_init", ESP_LOG_NONE);
}

void app_main(void)
{
  disable_module_logging();

  //Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  /* Configure a GPIO */
  ESP_LOGI(TAG, "***Configure LED GPIO ... ...");
  gpio_configure(LED_PIN);
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  ESP_LOGI(TAG, "***Start Chip Temp Sensor ... ...");
  ESP_ERROR_CHECK(temp_sensor_init());
  ESP_ERROR_CHECK(temp_sensor_enable());
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  /* Init the wifi */
  wifi_base_init();
#if CONFIG_WIFI_BASE_STA
  ESP_LOGI(TAG, "***ESP_WIFI_MODE_STA");
  wifi_init_sta();
  wifi_connect_sta();
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  ESP_LOGI(TAG, "***Start Web Server ... ...\n");
  start_webserver();

  // Get the network interface for the default Wi-Fi station interface
  esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  // Get the IP address
  esp_netif_ip_info_t ip_info;
  esp_netif_get_ip_info(netif, &ip_info);
  ESP_LOGI(TAG, "========");
  ESP_LOGI(TAG, "***ESP32 IP Address: " IPSTR, IP2STR(&ip_info.ip));/**< Interface IPV4 address */
  ESP_LOGI(TAG, "***ESP32 IP Netmask: " IPSTR, IP2STR(&ip_info.netmask));/**< Interface IPV4 netmask */
  ESP_LOGI(TAG, "***ESP32 IP gw:      " IPSTR, IP2STR(&ip_info.gw));/**< Interface IPV4 gateway address */
  ESP_LOGI(TAG, "========");

#if 0
  ESP_LOGI(TAG, "Send Whatsapp ... ...\n");
  send_whatsapp_message();
  vTaskDelay(5000 / portTICK_PERIOD_MS);
#endif

#else //CONFIG_WIFI_BASE_AP

#if CONFIG_WIFI_BASE_AP
  ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
  wifi_init_softap();
  wifi_connect_ap();
#endif
#endif

}

