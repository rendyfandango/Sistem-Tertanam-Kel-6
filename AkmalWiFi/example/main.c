#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "AkmalWiFi.h"

static const char *TAG = "main";

#define WIFI_SSID     "SSID_KAMU"
#define WIFI_PASSWORD "PASSWORD_KAMU"

void app_main(void)
{
    ESP_LOGI(TAG, "Starting WiFi connection...");

    esp_err_t ret = my_wifi_init_sta(WIFI_SSID, WIFI_PASSWORD);

    if (ret == ESP_OK) {
        // ---- Berhasil connect ----
        my_wifi_status_t status = my_wifi_get_status();

        printf("========================================\n");
        printf("  WiFi CONNECTED\n");
        printf("  SSID     : %s\n", WIFI_SSID);
        printf("  IP Address: %s\n", status.ipv4);
        printf("========================================\n");

        ESP_LOGI(TAG, "WiFi connected successfully, IP: %s", status.ipv4);

    } else if (ret == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Invalid SSID/password (check length - max 31/63 chars)");

    } else if (ret == ESP_FAIL) {
        ESP_LOGE(TAG, "Failed to connect - check SSID/password or AP availability");

    } else if (ret == ESP_ERR_TIMEOUT) {
        ESP_LOGE(TAG, "Connection timeout - AP not responding");

    } else {
        ESP_LOGE(TAG, "Unexpected error: %s", esp_err_to_name(ret));
    }

    // ---- Loop monitoring status tiap 10 detik (opsional, buat lihat kalau disconnect) ----
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));

        my_wifi_status_t status = my_wifi_get_status();
        if (status.connected) {
            ESP_LOGI(TAG, "Status: CONNECTED, IP: %s", status.ipv4);
        } else {
            ESP_LOGW(TAG, "Status: DISCONNECTED, retry_count: %d, last_error: %s",
                     status.retry_count, esp_err_to_name(status.last_error));
        }
    }
}