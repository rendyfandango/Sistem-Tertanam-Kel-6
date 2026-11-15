#include <string.h>
#include "bmp280_priv.h"

#define BUS_TIMEOUT_MS  100
#define SPI_MAX_PAYLOAD 31

esp_err_t bmp280_bus_read(struct bmp280_dev *d, uint8_t reg, uint8_t *buf, size_t len)
{
    if (d->bus == BMP280_BUS_I2C) {
        return i2c_master_transmit_receive(d->i2c, &reg, 1, buf, len, BUS_TIMEOUT_MS);
    }

    /* SPI: byte pertama = alamat dengan bit7 = 1 (read), lalu data keluar berurutan */
    if (len > SPI_MAX_PAYLOAD) {
        return ESP_ERR_INVALID_SIZE;
    }
    uint8_t tx[SPI_MAX_PAYLOAD + 1] = {0};
    uint8_t rx[SPI_MAX_PAYLOAD + 1] = {0};
    tx[0] = reg | 0x80;

    spi_transaction_t t = {
        .length    = (len + 1) * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    esp_err_t err = spi_device_transmit(d->spi, &t);
    if (err == ESP_OK) {
        memcpy(buf, rx + 1, len);   /* rx[0] = byte sampah saat kirim alamat */
    }
    return err;
}

esp_err_t bmp280_bus_write(struct bmp280_dev *d, uint8_t reg, uint8_t val)
{
    if (d->bus == BMP280_BUS_I2C) {
        uint8_t w[2] = { reg, val };
        return i2c_master_transmit(d->i2c, w, sizeof(w), BUS_TIMEOUT_MS);
    }

    /* SPI: bit7 alamat = 0 (write) */
    uint8_t tx[2] = { reg & 0x7F, val };
    spi_transaction_t t = {
        .length    = 16,
        .tx_buffer = tx,
    };
    return spi_device_transmit(d->spi, &t);
}