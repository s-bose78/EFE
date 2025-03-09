#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_cpu.h"
#include "esp_err.h"
#include "esp_partition.h"
#include "esp_flash.h"
#include "nvs_flash.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include <dirent.h>  // For directory operations
#include <string.h>
#include "esp_ota_ops.h"


const static char *TAG = "app_main";
char *mnt_point = "/sdcard";
#define SD_MOUNT_POINT "/sdcard"

#define FIRMWARE_FILE "/sdcard/HELLO.BIN"
uint8_t *firmware_data;
size_t firmware_size;


esp_err_t init_sd_card() {
    esp_err_t ret;

    // Mount SD card (assuming SPI interface)
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    // host slot 1
    host.slot = 1;  // Use the SDMMC host slot 1 for SD card access

    // SD Card Configuration
    slot_config.width = 1;
    slot_config.clk = CONFIG_EXAMPLE_PIN_CLK;
    slot_config.cmd = CONFIG_EXAMPLE_PIN_CMD;
    slot_config.d0  = CONFIG_EXAMPLE_PIN_D0;

    // Initialize the SD card using SDMMC
    sdmmc_card_t *card = NULL;
    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,   // Don't format if mount fails
        .max_files = 5,                    // Maximum number of open files
        .allocation_unit_size = 16 * 1024  // Allocation unit size (adjust as needed)
    };

    ret = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI("SD", "SD card initialized successfully");
    return ESP_OK;
}

esp_err_t find_file_sdcard(char *file_name)
{
    // List files in the root directory
    DIR *dir = opendir(SD_MOUNT_POINT);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory");
        return ESP_FAIL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(file_name, entry->d_name)) {
            ESP_LOGI(TAG, "\tFound file: %s", entry->d_name);
            closedir(dir);
            return ESP_OK;
        }
    }

    ESP_LOGE(TAG, "\tNot Found file: %s\n", file_name);
    closedir(dir);
    return ESP_FAIL;
}

esp_err_t cp_file_sdcard(char *file_name)
{
    FILE* f = fopen(file_name, "rb");
    if (f == NULL) {
        ESP_LOGE("Update", "Failed to open file. %s", file_name);
        return ESP_FAIL;
    }

    // Get the size of the firmware file
    fseek(f, 0, SEEK_END);  // Seek to the end to get the size
    firmware_size = ftell(f);  // Corrected: Use ftell() instead of fsize()
    fseek(f, 0, SEEK_SET);  // Seek back to the beginning

    ESP_LOGI("Update", "firmware_size %d",firmware_size);
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Allocate memory from PSRAM for the firmware
    firmware_data = (uint8_t *) heap_caps_malloc(firmware_size, MALLOC_CAP_SPIRAM);
    if (firmware_data == NULL) {
        ESP_LOGE("Update", "Failed to allocate memory for firmware.");
        fclose(f);
        return ESP_FAIL;
    }

    // Read the firmware into the allocated memory (PSRAM)
    fread(firmware_data, 1, firmware_size, f);
    fclose(f);

    return ESP_OK;
}

esp_err_t update_firmware()
{
    const char *L_TAG = "update_firmware";
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;

    esp_partition_iterator_t iter = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (iter != NULL) {
        const esp_partition_t *part = esp_partition_get(iter);
        ESP_LOGI(L_TAG, "Partition: %s, Type: %d, Subtype: %d, Address: 0x%x, Size: %d",
                 part->label, part->type, part->subtype, part->address, part->size);
        iter = esp_partition_next(iter);
    }

    // Find the first application partition (OTA partition 0)
    const esp_partition_t* update_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    if (update_partition == NULL) {
        ESP_LOGE(L_TAG, "Failed to find OTA partition.");
        return ESP_FAIL;
    } else {
        ESP_LOGI(L_TAG, "Using partition: %s (Type: %d, Subtype: %d) Address: 0x%x size: %d bytes", 
                update_partition->label, 
                update_partition->type, 
                update_partition->subtype,
                update_partition->address,
                update_partition->size);
    }
    err = esp_ota_begin(update_partition, 0, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(L_TAG, "Failed to esp_ota_begin. Error code: %s", esp_err_to_name(err));
        esp_ota_abort(update_handle);
        return err;
    }

    ESP_LOGI(L_TAG, "Going to write firmware of size %d to partition of size %d.", 
            firmware_size,
            update_partition->size);
    
    err = esp_ota_write(update_handle, (const void *)(firmware_data), firmware_size);
    if (err != ESP_OK) {
        esp_ota_abort(update_handle);
        ESP_LOGE(L_TAG, "Failed to esp_ota_write. Error code: %s", esp_err_to_name(err));
        return err;
    }
    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        } else {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(L_TAG, "Failed to set boot partition. Error code: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(L_TAG, "Boot partition set to %s", update_partition->label);

    return ESP_OK;
}

void app_main(void)
{
    int i;
    fflush(stdout);
    /*=============================== */
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    /*=============================== */
    /* Check PSRAM initialized and size */
    ESP_LOGI(TAG, "===> Check PSRAM Size=%d Init=%d", esp_psram_get_size(), esp_psram_is_initialized());
    vTaskDelay(pdMS_TO_TICKS(2000));  // Delay 1 second (1000 ms)
    /*=============================== */
    ESP_ERROR_CHECK(init_sd_card());
    ESP_LOGI(TAG, "===> Mount sdcard successful");
    vTaskDelay(pdMS_TO_TICKS(2000));  // Delay 1 second (1000 ms)
    /*=============================== */
    ESP_ERROR_CHECK(find_file_sdcard("HELLO.BIN"));
    ESP_LOGI(TAG, "===> Found %s", FIRMWARE_FILE); 
    vTaskDelay(pdMS_TO_TICKS(2000));  // Delay 1 second (1000 ms)
    /*=============================== */  
    ESP_ERROR_CHECK(cp_file_sdcard(FIRMWARE_FILE));
    ESP_LOGI(TAG, "===> Copied %s to local heap successfully", FIRMWARE_FILE); 
    vTaskDelay(pdMS_TO_TICKS(2000));  // Delay 1 second (1000 ms)
    /*=============================== */
    //ESP_ERROR_CHECK(update_firmware());
    if (update_firmware()) {
        return;
    }
    ESP_LOGI(TAG, "===> Update firmware Done");
    vTaskDelay(pdMS_TO_TICKS(2000));  // Delay 10 second before restart (1000 ms)
    /*=============================== */
    for (i = 6; i >= 0; i--) 
    {
        ESP_LOGI(TAG, "Restarting in %d seconds...", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();

    return;
}
