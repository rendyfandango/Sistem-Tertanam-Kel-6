#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool connected;
    uint8_t retry_count;
    esp_err_t last_error;
    char ipv4[16];
} my_wifi_status_t;

/**
 * @brief Inisialisasi dan connect WiFi STA ke AP yang ditentukan.
 *        Fungsi ini BLOCKING - nunggu sampai connected atau gagal/timeout.
 *
 * @param ssid     Nama WiFi target
 * @param password Password WiFi target
 * @return ESP_OK kalau berhasil connect, ESP_FAIL kalau gagal/timeout
 */
esp_err_t my_wifi_init_sta(const char *ssid, const char *password);

/**
 * @brief Cek status koneksi WiFi saat ini.
 * @return true kalau sedang connected (punya IP), false kalau tidak
 */
bool my_wifi_is_connected(void);

/**
 * @brief Ambil status koneksi WiFi STA saat ini secara thread-safe.
 *
 *        Semua field diambil dalam satu critical section, jadi hasilnya
 *        konsisten (bukan campuran dari momen yang berbeda-beda) walaupun
 *        fungsi ini dipanggil bersamaan dengan WiFi event yang sedang
 *        memproses connect/disconnect di background.
 *
 * @return my_wifi_status_t berisi:
 *         - connected    : true kalau sedang connected (punya IP)
 *         - retry_count  : jumlah percobaan reconnect sejak disconnect terakhir
 *         - last_error   : kode error terakhir (ESP_OK kalau tidak ada error)
 *         - ipv4         : alamat IP saat ini dalam format string ("0.0.0.0" kalau belum connect)
 */
my_wifi_status_t my_wifi_get_status(void);