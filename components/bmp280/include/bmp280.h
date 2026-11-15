#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BMP280_I2C_ADDR_LOW   0x76  /* SDO ke GND */
#define BMP280_I2C_ADDR_HIGH  0x77  /* SDO ke VCC */

/* Oversampling */
typedef enum {
    BMP280_OSRS_SKIP = 0,
    BMP280_OSRS_X1   = 1,
    BMP280_OSRS_X2   = 2,
    BMP280_OSRS_X4   = 3,
    BMP280_OSRS_X8   = 4,
    BMP280_OSRS_X16  = 5,
} bmp280_osrs_t;

/* IIR filter */
typedef enum {
    BMP280_FILTER_OFF = 0,
    BMP280_FILTER_X2  = 1,
    BMP280_FILTER_X4  = 2,
    BMP280_FILTER_X8  = 3,
    BMP280_FILTER_X16 = 4,
} bmp280_filter_t;

/* Waktu standby (hanya berlaku di mode NORMAL) */
typedef enum {
    BMP280_STANDBY_0_5_MS  = 0,
    BMP280_STANDBY_62_5_MS = 1,
    BMP280_STANDBY_125_MS  = 2,
    BMP280_STANDBY_250_MS  = 3,
    BMP280_STANDBY_500_MS  = 4,
    BMP280_STANDBY_1000_MS = 5,
    BMP280_STANDBY_2000_MS = 6,
    BMP280_STANDBY_4000_MS = 7,
} bmp280_standby_t;

/* NORMAL = sensor ukur terus-menerus.
 * FORCED/SLEEP = sekali ukur tiap bmp280_read() dipanggil. */
typedef enum {
    BMP280_MODE_SLEEP  = 0,
    BMP280_MODE_FORCED = 1,
    BMP280_MODE_NORMAL = 3,
} bmp280_mode_t;

typedef struct {
    bmp280_mode_t    mode;
    bmp280_osrs_t    osrs_t;   /* tidak boleh SKIP (suhu dipakai untuk hitung tekanan) */
    bmp280_osrs_t    osrs_p;
    bmp280_filter_t  filter;
    bmp280_standby_t standby;
} bmp280_settings_t;

#define BMP280_SETTINGS_DEFAULT() {            \
    .mode    = BMP280_MODE_NORMAL,             \
    .osrs_t  = BMP280_OSRS_X2,                 \
    .osrs_p  = BMP280_OSRS_X16,                \
    .filter  = BMP280_FILTER_X4,               \
    .standby = BMP280_STANDBY_62_5_MS,         \
}

typedef struct {
    spi_host_device_t host;     /* contoh: SPI2_HOST (bus harus sudah di-init) */
    gpio_num_t        cs_io;
    int               clock_hz; /* 0 = default 1 MHz (maks BMP280 = 10 MHz) */
} bmp280_spi_config_t;

typedef struct {
    i2c_master_bus_handle_t bus; /* bus harus sudah dibuat */
    uint8_t                 addr;
    uint32_t                scl_hz; /* 0 = default 400 kHz */
} bmp280_i2c_config_t;

typedef struct {
    float temperature_c;  /* derajat Celsius */
    float pressure_pa;    /* Pascal (NAN jika osrs_p = SKIP) */
} bmp280_data_t;

typedef struct bmp280_dev *bmp280_handle_t;

/* Buat device lewat SPI. settings boleh NULL (pakai default). */
esp_err_t bmp280_new_spi(const bmp280_spi_config_t *cfg,
                         const bmp280_settings_t *settings,
                         bmp280_handle_t *out_handle);

/* Buat device lewat I2C. settings boleh NULL (pakai default). */
esp_err_t bmp280_new_i2c(const bmp280_i2c_config_t *cfg,
                         const bmp280_settings_t *settings,
                         bmp280_handle_t *out_handle);

/* Lepas device dan bebaskan memori. */
esp_err_t bmp280_del(bmp280_handle_t dev);

/* Ubah konfigurasi saat runtime. */
esp_err_t bmp280_set_settings(bmp280_handle_t dev, const bmp280_settings_t *settings);

/* Baca suhu & tekanan. Thread-safe (ada mutex internal). */
esp_err_t bmp280_read(bmp280_handle_t dev, bmp280_data_t *out);

/* Estimasi ketinggian (meter) dari tekanan.
 * sea_level_pa: tekanan permukaan laut, standar 101325.0 */
float bmp280_pressure_to_altitude(float pressure_pa, float sea_level_pa);

#ifdef __cplusplus
}
#endif