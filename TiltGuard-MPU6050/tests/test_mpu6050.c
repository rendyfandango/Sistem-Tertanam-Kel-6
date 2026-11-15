/* Logic and API-contract tests only: these mocks perform no hardware I/O. */
#include <assert.h>
#include <stdio.h>
#include "mpu6050.h"

struct test_bus { int unused; };
struct test_device { int unused; };
static struct test_bus bus;
static struct test_device endpoint;
static esp_err_t add_result = ESP_OK, read_result = ESP_OK, remove_result = ESP_OK;
static uint8_t returned_id = 0x68;
static unsigned add_calls, remove_calls;
static uint16_t expected_address = 0x68;

esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t b,
    const i2c_device_config_t *config, i2c_master_dev_handle_t *device)
{
    ++add_calls;
    assert(b == &bus);
    assert(config->device_address == expected_address);
    assert(config->dev_addr_length == I2C_ADDR_BIT_LEN_7);
    assert(config->scl_speed_hz == 400000);
    if (add_result == ESP_OK) *device = &endpoint;
    return add_result;
}

esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t device,
    const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len, int timeout_ms)
{
    assert(device == &endpoint);
    assert(tx_len == 1 && rx_len == 1 && tx[0] == 0x75);
    assert(timeout_ms == 100);
    *rx = returned_id; /* Even an error may touch the SDK receive buffer. */
    return read_result;
}

esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t device)
{
    ++remove_calls;
    assert(device == &endpoint);
    return remove_result;
}

int main(void)
{
    mpu6050_config_t config = MPU6050_CONFIG_DEFAULT();
    mpu6050_handle_t sensor = NULL;
    uint8_t identity = 0xaa;
    assert(mpu6050_init(NULL, &config, &sensor) == ESP_ERR_INVALID_ARG);
    assert(mpu6050_init(&bus, NULL, &sensor) == ESP_ERR_INVALID_ARG);
    assert(mpu6050_init(&bus, &config, NULL) == ESP_ERR_INVALID_ARG);
    config.address = 0x70;
    assert(mpu6050_init(&bus, &config, &sensor) == ESP_ERR_INVALID_ARG);
    config = (mpu6050_config_t)MPU6050_CONFIG_DEFAULT();
    config.scl_speed_hz = 400001;
    assert(mpu6050_init(&bus, &config, &sensor) == ESP_ERR_INVALID_ARG);
    config.scl_speed_hz = 400000;
    config.timeout_ms = -1;
    assert(mpu6050_init(&bus, &config, &sensor) == ESP_ERR_INVALID_ARG);
    config.timeout_ms = 100;
    assert(add_calls == 0 && sensor == NULL);

    add_result = ESP_ERR_NO_MEM;
    assert(mpu6050_init(&bus, &config, &sensor) == ESP_ERR_NO_MEM);
    assert(sensor == NULL);
    add_result = ESP_OK;
    assert(mpu6050_init(&bus, &config, &sensor) == ESP_OK);
    assert(mpu6050_init(&bus, &config, &sensor) == ESP_ERR_INVALID_ARG);
    assert(mpu6050_check_connection(NULL, &identity) == ESP_ERR_INVALID_ARG);
    assert(mpu6050_check_connection(sensor, NULL) == ESP_ERR_INVALID_ARG);
    assert(mpu6050_check_connection(sensor, &identity) == ESP_OK && identity == 0x68);

    returned_id = 0x00;
    assert(mpu6050_check_connection(sensor, &identity) == ESP_ERR_INVALID_RESPONSE);
    assert(identity == 0x00);
    read_result = ESP_ERR_TIMEOUT;
    returned_id = 0x68;
    identity = 0xaa;
    assert(mpu6050_check_connection(sensor, &identity) == ESP_ERR_TIMEOUT);
    assert(identity == 0xaa);
    read_result = ESP_FAIL;
    assert(mpu6050_check_connection(sensor, &identity) == ESP_FAIL);
    assert(identity == 0xaa);
    read_result = ESP_OK;

    mpu6050_handle_t saved = sensor;
    remove_result = ESP_FAIL;
    assert(mpu6050_deinit(&sensor) == ESP_FAIL && sensor == saved);
    remove_result = ESP_OK;
    assert(mpu6050_deinit(&sensor) == ESP_OK && sensor == NULL);
    assert(mpu6050_deinit(&sensor) == ESP_ERR_INVALID_ARG);
    assert(mpu6050_deinit(NULL) == ESP_ERR_INVALID_ARG);

    expected_address = config.address = 0x69;
    assert(mpu6050_init(&bus, &config, &sensor) == ESP_OK);
    assert(mpu6050_check_connection(sensor, &identity) == ESP_OK && identity == 0x68);
    assert(mpu6050_deinit(&sensor) == ESP_OK);
    assert(add_calls == 3 && remove_calls == 3);
    puts("PASS: driver contracts, identity, transfer errors, and device lifecycle (mock I2C)");
    return 0;
}
