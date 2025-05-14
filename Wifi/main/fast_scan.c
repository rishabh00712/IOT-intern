#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"          // Wi-Fi driver
#include "esp_event.h"         // Event loop
#include "esp_log.h"           // Logging
#include "nvs_flash.h"         // Non-volatile storage (used for Wi-Fi credentials)

#define WIFI_SSID "library"    // Change this to your Wi-Fi SSID
#define WIFI_PASS ""           // Password (leave "" for open networks)

static const char *TAG = "wifi_simple";  // Tag used in logging

// Event handler function to manage Wi-Fi and IP events
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        // When station starts, attempt to connect
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        // If disconnected, try reconnecting
        ESP_LOGI(TAG, "Disconnected, reconnecting...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // When IP is received from AP, print it
        ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected with IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

// Initialize and start Wi-Fi in station mode
void wifi_init_sta(void) {
    // Initialize TCP/IP stack and create default event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default Wi-Fi station interface
    esp_netif_create_default_wifi_sta();

    // Initialize Wi-Fi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers for Wi-Fi and IP events
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // Set Wi-Fi configuration (SSID and password)
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    // Set Wi-Fi mode to station (client)
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // Apply the configuration to the Wi-Fi driver
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Start the Wi-Fi driver
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi init done");
}

// Main entry point
void app_main(void) {
    // Initialize NVS (used by Wi-Fi to store credentials)
    esp_err_t ret = nvs_flash_init();

    // If NVS is full or version changed, erase and re-init
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);  // Check NVS init result

    // Start Wi-Fi connection
    wifi_init_sta();
}
