#include <stdio.h>

#include "esp_log.h"
#include "esp_err.h"
#include "driver/spi_master.h"

#include "microsd.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    esp_err_t ret;

    // =====================================
    // MICROSD CONFIGURATION
    // =====================================

    microsd_config_t sd_config = {
        .host = SPI2_HOST,

        // Example pins only.
        // Adjust according to actual wiring.
        .pin_mosi = 4,
        .pin_miso = 5,
        .pin_sck = 6,
        .pin_cs = 7
    };

    // =====================================
    // INITIALIZE MICROSD
    // =====================================

    ESP_LOGI(TAG, "Initializing MicroSD...");

    ret = microsd_init(&sd_config);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MicroSD initialization failed: %s",
                 esp_err_to_name(ret));
        return;
    }

    // =====================================
    // WRITE FILE
    // =====================================

    ESP_LOGI(TAG, "Writing file...");

    ret = microsd_write_file(
        "/sdcard/data.csv",
        "Time,Roll,Pitch,Shock"
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Write failed: %s",
                 esp_err_to_name(ret));
        goto cleanup;
    }

    // =====================================
    // APPEND DATA
    // =====================================

    ESP_LOGI(TAG, "Appending data...");

    ret = microsd_append_line(
        "/sdcard/data.csv",
        "1,10.5,20.3,0"
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Append failed: %s",
                 esp_err_to_name(ret));
        goto cleanup;
    }

    ret = microsd_append_line(
        "/sdcard/data.csv",
        "2,12.5,22.3,1"
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Append failed: %s",
                 esp_err_to_name(ret));
        goto cleanup;
    }

    // =====================================
    // READ FILE
    // =====================================

    ESP_LOGI(TAG, "Reading file...");

    char buffer[512];

    ret = microsd_read_file(
        "/sdcard/data.csv",
        buffer,
        sizeof(buffer)
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read failed: %s",
                 esp_err_to_name(ret));
        goto cleanup;
    }

    ESP_LOGI(TAG, "MicroSD content:\n%s", buffer);

    ESP_LOGI(TAG, "MicroSD test completed!");

cleanup:

    // =====================================
    // UNMOUNT
    // =====================================

    ret = microsd_unmount();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Unmount failed: %s",
                 esp_err_to_name(ret));
    }
}