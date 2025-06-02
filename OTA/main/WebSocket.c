#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"

// Tags
static const char *TAG_WIFI = "wifi_setup";
static const char *TAG_OTA = "firebase_ota";

// Firmware version
#define CURRENT_FIRMWARE_VERSION "v1.0.0"

// Wi-Fi credentials
#define WIFI_SSID "Sanjoy"
#define WIFI_PASS "daddydaddy"

// Firebase config
#define FIREBASE_HOST "iot-bin-database-default-rtdb.firebaseio.com"
#define FIREBASE_PATH "/firmware.json"
#define FIREBASE_AUTH "8Eru70ElyOzwf7w8bL36uKxnY5HDOBrmKCuqZZwZ"

// JSON response buffer
static char response_buffer[1024];
static int response_len = 0;

// Event group for Wi-Fi connection
#define WIFI_CONNECTED_BIT BIT0
static EventGroupHandle_t wifi_event_group;

// Wi-Fi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG_WIFI, "Connected to Wi-Fi.");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGW(TAG_WIFI, "Disconnected. Retrying...");
    }
}

// Initialize Wi-Fi in STA mode
void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Wait until connected
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
}

// Handle HTTP events
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                if (response_len + evt->data_len < sizeof(response_buffer)) {
                    memcpy(response_buffer + response_len, evt->data, evt->data_len);
                    response_len += evt->data_len;
                    response_buffer[response_len] = 0;
                }
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

// Perform OTA update from given firmware URL
void perform_ota_update(const char *firmware_url)
{
    esp_http_client_config_t http_config = {
        .url = firmware_url,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    ESP_LOGI(TAG_OTA, "Starting OTA from: %s", firmware_url);

    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG_OTA, "OTA successful, restarting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG_OTA, "OTA failed: %s", esp_err_to_name(ret));
    }
}

// Firebase GET and OTA check (single call)
void check_and_perform_ota(void)
{
    response_len = 0;

    char url[256];
    snprintf(url, sizeof(url),
             "https://%s%s?auth=%s",
             FIREBASE_HOST, FIREBASE_PATH, FIREBASE_AUTH);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .skip_cert_common_name_check = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG_OTA, "HTTPS Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));

        cJSON *root = cJSON_Parse(response_buffer);
        if (root) {
            const cJSON *url = cJSON_GetObjectItem(root, "url");
            const cJSON *version = cJSON_GetObjectItem(root, "version");

            if (url && cJSON_IsString(url) &&
                version && cJSON_IsString(version)) {

                ESP_LOGI(TAG_OTA, "Available firmware: %s (version: %s)", url->valuestring, version->valuestring);
                ESP_LOGI(TAG_OTA, "Current firmware version: %s", CURRENT_FIRMWARE_VERSION);

                if (strcmp(CURRENT_FIRMWARE_VERSION, version->valuestring) != 0) {
                    ESP_LOGI(TAG_OTA, "Firmware update required. Starting OTA...");
                    perform_ota_update(url->valuestring);
                } else {
                    ESP_LOGI(TAG_OTA, "Firmware is up to date. No OTA required.");
                }

            } else {
                ESP_LOGE(TAG_OTA, "Invalid JSON: Missing 'url' or 'version'");
            }

            cJSON_Delete(root);
        } else {
            ESP_LOGE(TAG_OTA, "JSON parsing failed");
        }

    } else {
        ESP_LOGE(TAG_OTA, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

// Main app entry
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();
    vTaskDelay(pdMS_TO_TICKS(1000));  // optional short delay

    ESP_LOGI(TAG_WIFI, "Wi-Fi connected. Starting OTA check...");
    check_and_perform_ota();  // run once synchronously

    ESP_LOGI(TAG_OTA, "OTA check complete. Continuing with application logic...");

}
