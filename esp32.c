#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_app_desc.h"
#include "protocol_examples_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "OTA_MAIN"
#define VERSION_CHECK_URL "https://your-server.com/version.json"     // your Firebase endpoint
#define CURRENT_FIRMWARE_VERSION esp_app_get_description()->version

extern const uint8_t server_cert_pem_start[] asm("_binary_server_cert_pem_start");
extern const uint8_t server_cert_pem_end[]   asm("_binary_server_cert_pem_end");

void check_and_update_task(void *pvParameter)
{
    //Logs the start of the OTA check process.
    ESP_LOGI(TAG, "Checking for OTA update...");

    // Step 1: Prepares an HTTPS client config.
    esp_http_client_config_t version_config = {
        .url = VERSION_CHECK_URL,
        .cert_pem = (char *)server_cert_pem_start,//embedded certificate for security 
    };

    //Initializes the HTTP client and performs the GET request to download the file.
    esp_http_client_handle_t client = esp_http_client_init(&version_config);
    esp_http_client_perform(client);

    //Reads the JSON response into a buffer
    char buffer[128];
    int len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
    buffer[len] = 0;
    //Cleans up the HTTP connection.
    esp_http_client_cleanup(client);
    //......................................................
    // Assume JSON: {"version":"1.1.0", "url":"https://your-server.com/firmware.bin"}
    char *version = strstr(buffer, "\"version\":\"") + 11;
    char *version_end = strchr(version, '"');
    *version_end = '\0';

    char *firmware_url = strstr(version_end + 1, "\"url\":\"") + 7;
    char *url_end = strchr(firmware_url, '"');
    *url_end = '\0';

    ESP_LOGI(TAG, "Current version: %s", CURRENT_FIRMWARE_VERSION);
    ESP_LOGI(TAG, "Available version: %s", version);

    if (strcmp(version, CURRENT_FIRMWARE_VERSION) != 0) {
        ESP_LOGI(TAG, "Version mismatch, proceeding with OTA...");

        //Prepares the OTA download request with the .bin firmware URL.
        esp_http_client_config_t ota_config = {
            .url = firmware_url,
            .cert_pem = (char *)server_cert_pem_start,
        };
        //Calls esp_https_ota(), which handles downloading and flashing the firmware.
        esp_err_t ret = esp_https_ota(&ota_config);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "OTA update successful. Rebooting...");
            esp_restart();
        } else {
            ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGI(TAG, "Firmware is up-to-date.");
    }

    vTaskDelete(NULL); // delete the task
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());  // connects to WiFi from example_common

    xTaskCreate(&check_and_update_task, "ota_check", 8192, NULL, 5, NULL);
}
