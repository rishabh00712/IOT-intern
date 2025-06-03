#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include<string.h>


void app_main() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle_t my_handle; // open namespace in read/write mode
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    ESP_ERROR_CHECK(err);

    // const char *version = "1.0.0";
    // ESP_ERROR_CHECK(nvs_set_str(my_handle, "version", version));
    // ESP_ERROR_CHECK(nvs_commit(my_handle));
    // ESP_LOGI("NVS", "Stored version: %s", version);


    // reading a string from nvs
    size_t required_size;
    err = nvs_get_str(my_handle, "version", NULL, &required_size);
    if(err == ESP_OK && required_size > 0) {
        char *stored_version = malloc(required_size);
        err = nvs_get_str(my_handle, "version", stored_version, &required_size);
        if (err == ESP_OK) {
            ESP_LOGI("NVS", "Read version from NVS: %s", stored_version);
            if(strcmp(stored_version, "1.0.1") != 0) {
                ESP_LOGI("NVS", "Version mismatch, updating...");
                // Perform OTA update or other actions
            } else {
                ESP_LOGI("NVS", "Version is up to date.");
            }
        } else {
            ESP_LOGE("NVS", "Failed to read version from NVS");
        }
        free(stored_version);
    }



    // storing a number ot nvs
    // err = nvs_set_i32(my_handle, "counter", 123);
    // ESP_ERROR_CHECK(err);
    // ESP_ERROR_CHECK(nvs_commit(my_handle));


    // reading a number from nvs(its kinda easy)
    int32_t read_val = 0;
    err = nvs_get_i32(my_handle, "counter", &read_val);
    if (err == ESP_OK) {
        printf("Read from NVS: %ld\n", read_val);
    }

    nvs_close(my_handle);
}

