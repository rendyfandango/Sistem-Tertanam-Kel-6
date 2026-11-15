#include "mpu6050.h"
#include "mpu6050_regs.h"
#include <stdlib.h>

struct mpu6050_device {
    i2c_master_dev_handle_t i2c;
    int timeout_ms;
};

/* transmit_receive keeps the register-pointer write and read in one transaction
 * with a repeated START; another device cannot interleave those two steps.
 */
static esp_err_t mpu6050_read_reg(mpu6050_handle_t device, uint8_t reg, uint8_t *value)
{
    uint8_t received = 0;
    esp_err_t err = i2c_master_transmit_receive(device->i2c, &reg, sizeof(reg),
                                              &received, sizeof(received), device->timeout_ms);
    if (err == ESP_OK) {
        *value = received; /* Never return an untrustworthy byte after a timeout. */
    }
    return err;
}

esp_err_t mpu6050_init(i2c_master_bus_handle_t bus,
                       const mpu6050_config_t *config, mpu6050_handle_t *out)
{
    if (bus == NULL || config == NULL || out == NULL || *out != NULL ||
        (config->address != 0x68 && config->address != 0x69) ||
        config->scl_speed_hz < 10000 || config->scl_speed_hz > 400000 ||
        config->timeout_ms <= 0 || config->timeout_ms > 1000) {
        return ESP_ERR_INVALID_ARG;
    }
    mpu6050_handle_t device = calloc(1, sizeof(*device));
    if (device == NULL) {
        return ESP_ERR_NO_MEM;
    }
    const i2c_device_config_t i2c_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = config->address,
        .scl_speed_hz = config->scl_speed_hz,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &i2c_config, &device->i2c);
    if (err != ESP_OK) {
        free(device);
        return err;
    }
    device->timeout_ms = config->timeout_ms;
    *out = device;
    return ESP_OK;
}

esp_err_t mpu6050_check_connection(mpu6050_handle_t device, uint8_t *who_am_i)
{
    if (device == NULL || who_am_i == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = mpu6050_read_reg(device, MPU6050_REG_WHO_AM_I, who_am_i);
    if (err != ESP_OK) {
        return err;
    }
    return (*who_am_i == MPU6050_WHO_AM_I_EXPECTED) ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}

esp_err_t mpu6050_deinit(mpu6050_handle_t *device)
{
    if (device == NULL || *device == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = i2c_master_bus_rm_device((*device)->i2c);
    if (err == ESP_OK) {
        free(*device);
        *device = NULL;
    }
    return err;
}
