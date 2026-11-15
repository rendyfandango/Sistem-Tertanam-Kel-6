#include <math.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bmp280.h"
#include "bmp280_priv.h"
#include "bmp280_regs.h"

static const char *TAG = "bmp280";
#define LOCK_TIMEOUT pdMS_TO_TICKS(200)

/* ---------- helper waktu pengukuran ---------- */

static inline uint32_t osr_factor(bmp280_osrs_t o)
{
    return (o == BMP280_OSRS_SKIP) ? 0 : (1u << (o - 1));
}

/* Waktu maksimum konversi (datasheet): 1.25 + 2.3*Ot + (2.3*Op + 0.575) ms */
static uint32_t measure_time_ms(const bmp280_settings_t *s)
{
    uint32_t ot = osr_factor(s->osrs_t);
    uint32_t op = osr_factor(s->osrs_p);
    float t = 1.25f;
    if (ot) t += 2.3f * ot;
    if (op) t += 2.3f * op + 0.575f;
    return (uint32_t)ceilf(t);
}

static esp_err_t wait_measurement(struct bmp280_dev *d)
{
    vTaskDelay(pdMS_TO_TICKS(measure_time_ms(&d->settings)) + 1);
    for (int i = 0; i < 20; i++) {
        uint8_t st = 0;
        ESP_RETURN_ON_ERROR(bmp280_bus_read(d, BMP280_REG_STATUS, &st, 1), TAG, "read status");
        if (!(st & BMP280_STATUS_MEASURING)) {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(2) + 1);
    }
    return ESP_ERR_TIMEOUT;
}

/* ---------- kalibrasi & kompensasi (rumus datasheet) ---------- */

static esp_err_t read_calibration(struct bmp280_dev *d)
{
    uint8_t b[BMP280_CALIB_LEN];
    ESP_RETURN_ON_ERROR(bmp280_bus_read(d, BMP280_REG_CALIB, b, sizeof(b)), TAG, "read calib");

#define U16(i) ((uint16_t)((uint16_t)b[i] | ((uint16_t)b[(i) + 1] << 8)))
#define S16(i) ((int16_t)U16(i))
    bmp280_calib_t *c = &d->calib;
    c->T1 = U16(0);  c->T2 = S16(2);  c->T3 = S16(4);
    c->P1 = U16(6);  c->P2 = S16(8);  c->P3 = S16(10);
    c->P4 = S16(12); c->P5 = S16(14); c->P6 = S16(16);
    c->P7 = S16(18); c->P8 = S16(20); c->P9 = S16(22);
#undef U16
#undef S16
    return ESP_OK;
}

/* Hasil: suhu dalam 0.01 °C. Sekaligus mengisi t_fine. */
static int32_t compensate_temp(struct bmp280_dev *d, int32_t adc_T)
{
    const bmp280_calib_t *c = &d->calib;
    int32_t var1 = (((adc_T >> 3) - ((int32_t)c->T1 << 1)) * (int32_t)c->T2) >> 11;
    int32_t var2 = (((((adc_T >> 4) - (int32_t)c->T1) *
                      ((adc_T >> 4) - (int32_t)c->T1)) >> 12) * (int32_t)c->T3) >> 14;
    d->t_fine = var1 + var2;
    return (d->t_fine * 5 + 128) >> 8;
}

/* Hasil: tekanan Pa dalam format Q24.8 (bagi 256 untuk Pa). 0 = gagal. */
static uint32_t compensate_press(struct bmp280_dev *d, int32_t adc_P)
{
    const bmp280_calib_t *c = &d->calib;
    int64_t var1 = (int64_t)d->t_fine - 128000;
    int64_t var2 = var1 * var1 * (int64_t)c->P6;
    var2 = var2 + ((var1 * (int64_t)c->P5) << 17);
    var2 = var2 + (((int64_t)c->P4) << 35);
    var1 = ((var1 * var1 * (int64_t)c->P3) >> 8) + ((var1 * (int64_t)c->P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) * (int64_t)c->P1) >> 33;
    if (var1 == 0) {
        return 0;   /* hindari pembagian nol */
    }
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)c->P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)c->P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)c->P7) << 4);
    return (uint32_t)p;
}

/* ---------- konfigurasi ---------- */

static esp_err_t apply_locked(struct bmp280_dev *d, const bmp280_settings_t *s)
{
    ESP_RETURN_ON_FALSE(s->osrs_t != BMP280_OSRS_SKIP, ESP_ERR_INVALID_ARG, TAG,
                        "osrs_t tidak boleh SKIP");

    uint8_t ctrl = (uint8_t)((s->osrs_t << 5) | (s->osrs_p << 2));

    /* Datasheet: register config hanya aman ditulis saat mode sleep */
    ESP_RETURN_ON_ERROR(bmp280_bus_write(d, BMP280_REG_CTRL_MEAS, ctrl), TAG, "write ctrl (sleep)");
    ESP_RETURN_ON_ERROR(bmp280_bus_write(d, BMP280_REG_CONFIG,
                        (uint8_t)((s->standby << 5) | (s->filter << 2))), TAG, "write config");
    d->settings = *s;

    if (s->mode == BMP280_MODE_NORMAL) {
        ESP_RETURN_ON_ERROR(bmp280_bus_write(d, BMP280_REG_CTRL_MEAS, ctrl | BMP280_MODE_NORMAL),
                            TAG, "write ctrl (normal)");
        return wait_measurement(d);   /* tunggu data pertama siap */
    }
    return ESP_OK;
}

static esp_err_t chip_setup(struct bmp280_dev *d, const bmp280_settings_t *settings)
{
    bmp280_settings_t s = BMP280_SETTINGS_DEFAULT();
    if (settings) s = *settings;

    d->lock = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(d->lock, ESP_ERR_NO_MEM, TAG, "mutex");

    /* Soft reset */
    ESP_RETURN_ON_ERROR(bmp280_bus_write(d, BMP280_REG_RESET, BMP280_RESET_CMD), TAG, "reset");
    vTaskDelay(pdMS_TO_TICKS(10) + 1);

    /* Cek chip ID */
    uint8_t id = 0;
    ESP_RETURN_ON_ERROR(bmp280_bus_read(d, BMP280_REG_ID, &id, 1), TAG, "read id");
    if (id == BMP280_CHIP_ID_BME280) {
        ESP_LOGW(TAG, "Chip ID 0x60 = BME280 (ada humidity). Driver ini hanya baca suhu/tekanan.");
    } else if (id != BMP280_CHIP_ID_BMP280 && id != BMP280_CHIP_ID_SAMPLE_1 &&
               id != BMP280_CHIP_ID_SAMPLE_2) {
        ESP_LOGE(TAG, "Chip ID salah: 0x%02X (harusnya 0x58). Cek wiring/mode bus.", id);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "Chip ID: 0x%02X", id);

    /* Tunggu kalibrasi selesai disalin ke register (im_update = 0) */
    for (int i = 0; i < 10; i++) {
        uint8_t st = 0;
        ESP_RETURN_ON_ERROR(bmp280_bus_read(d, BMP280_REG_STATUS, &st, 1), TAG, "status");
        if (!(st & BMP280_STATUS_IM_UPDATE)) break;
        vTaskDelay(pdMS_TO_TICKS(2) + 1);
    }

    ESP_RETURN_ON_ERROR(read_calibration(d), TAG, "calibration");
    return apply_locked(d, &s);
}

/* ---------- API publik ---------- */

esp_err_t bmp280_new_spi(const bmp280_spi_config_t *cfg,
                         const bmp280_settings_t *settings,
                         bmp280_handle_t *out_handle)
{
    ESP_RETURN_ON_FALSE(cfg && out_handle, ESP_ERR_INVALID_ARG, TAG, "null arg");

    struct bmp280_dev *d = calloc(1, sizeof(*d));
    ESP_RETURN_ON_FALSE(d, ESP_ERR_NO_MEM, TAG, "no mem");
    d->bus = BMP280_BUS_SPI;

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = cfg->clock_hz ? cfg->clock_hz : 1000000,
        .mode           = 0,
        .spics_io_num   = cfg->cs_io,
        .queue_size     = 1,
    };
    esp_err_t err = spi_bus_add_device(cfg->host, &devcfg, &d->spi);
    if (err == ESP_OK) err = chip_setup(d, settings);
    if (err != ESP_OK) {
        bmp280_del(d);
        return err;
    }
    *out_handle = d;
    return ESP_OK;
}

esp_err_t bmp280_new_i2c(const bmp280_i2c_config_t *cfg,
                         const bmp280_settings_t *settings,
                         bmp280_handle_t *out_handle)
{
    ESP_RETURN_ON_FALSE(cfg && cfg->bus && out_handle, ESP_ERR_INVALID_ARG, TAG, "null arg");

    struct bmp280_dev *d = calloc(1, sizeof(*d));
    ESP_RETURN_ON_FALSE(d, ESP_ERR_NO_MEM, TAG, "no mem");
    d->bus = BMP280_BUS_I2C;

    i2c_device_config_t devcfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = cfg->addr ? cfg->addr : BMP280_I2C_ADDR_LOW,
        .scl_speed_hz    = cfg->scl_hz ? cfg->scl_hz : 400000,
    };
    esp_err_t err = i2c_master_bus_add_device(cfg->bus, &devcfg, &d->i2c);
    if (err == ESP_OK) err = chip_setup(d, settings);
    if (err != ESP_OK) {
        bmp280_del(d);
        return err;
    }
    *out_handle = d;
    return ESP_OK;
}

esp_err_t bmp280_del(bmp280_handle_t dev)
{
    ESP_RETURN_ON_FALSE(dev, ESP_ERR_INVALID_ARG, TAG, "null handle");
    if (dev->lock) {
        bmp280_bus_write(dev, BMP280_REG_CTRL_MEAS, 0x00);  /* balik ke sleep */
        vSemaphoreDelete(dev->lock);
    }
    if (dev->spi) spi_bus_remove_device(dev->spi);
    if (dev->i2c) i2c_master_bus_rm_device(dev->i2c);
    free(dev);
    return ESP_OK;
}

esp_err_t bmp280_set_settings(bmp280_handle_t dev, const bmp280_settings_t *settings)
{
    ESP_RETURN_ON_FALSE(dev && settings, ESP_ERR_INVALID_ARG, TAG, "null arg");
    if (xSemaphoreTake(dev->lock, LOCK_TIMEOUT) != pdTRUE) return ESP_ERR_TIMEOUT;
    esp_err_t err = apply_locked(dev, settings);
    xSemaphoreGive(dev->lock);
    return err;
}

static esp_err_t read_locked(struct bmp280_dev *d, bmp280_data_t *out)
{
    const bmp280_settings_t *s = &d->settings;

    if (s->mode != BMP280_MODE_NORMAL) {
        /* Mode on-demand: picu satu pengukuran lalu tunggu selesai */
        uint8_t ctrl = (uint8_t)((s->osrs_t << 5) | (s->osrs_p << 2) | BMP280_MODE_FORCED);
        ESP_RETURN_ON_ERROR(bmp280_bus_write(d, BMP280_REG_CTRL_MEAS, ctrl), TAG, "trigger");
        ESP_RETURN_ON_ERROR(wait_measurement(d), TAG, "wait");
    }

    uint8_t raw[6];
    ESP_RETURN_ON_ERROR(bmp280_bus_read(d, BMP280_REG_PRESS_MSB, raw, sizeof(raw)), TAG, "read data");

    int32_t adc_p = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_t = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);

    out->temperature_c = compensate_temp(d, adc_t) / 100.0f;   /* wajib dulu: isi t_fine */

    if (s->osrs_p == BMP280_OSRS_SKIP) {
        out->pressure_pa = NAN;
    } else {
        uint32_t p = compensate_press(d, adc_p);
        if (p == 0) return ESP_ERR_INVALID_RESPONSE;
        out->pressure_pa = p / 256.0f;
    }
    return ESP_OK;
}

esp_err_t bmp280_read(bmp280_handle_t dev, bmp280_data_t *out)
{
    ESP_RETURN_ON_FALSE(dev && out, ESP_ERR_INVALID_ARG, TAG, "null arg");
    if (xSemaphoreTake(dev->lock, LOCK_TIMEOUT) != pdTRUE) return ESP_ERR_TIMEOUT;
    esp_err_t err = read_locked(dev, out);
    xSemaphoreGive(dev->lock);
    return err;
}

float bmp280_pressure_to_altitude(float pressure_pa, float sea_level_pa)
{
    return 44330.0f * (1.0f - powf(pressure_pa / sea_level_pa, 0.1903f));
}