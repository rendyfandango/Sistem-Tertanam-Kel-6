#pragma once

#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "bmp280.h"

typedef enum {
    BMP280_BUS_SPI,
    BMP280_BUS_I2C,
} bmp280_bus_type_t;

typedef struct {
    uint16_t T1; int16_t T2, T3;
    uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
} bmp280_calib_t;

struct bmp280_dev {
    bmp280_bus_type_t       bus;
    spi_device_handle_t     spi;
    i2c_master_dev_handle_t i2c;
    SemaphoreHandle_t       lock;
    bmp280_settings_t       settings;
    bmp280_calib_t          calib;
    int32_t                 t_fine;
};

esp_err_t bmp280_bus_read(struct bmp280_dev *d, uint8_t reg, uint8_t *buf, size_t len);
esp_err_t bmp280_bus_write(struct bmp280_dev *d, uint8_t reg, uint8_t val);