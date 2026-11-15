#include <stdbool.h>
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mpu6050.h"

static const char *TAG = "MPU6050";

void app_main(void)
{
    /* The application owns this bus; the future SSD1306 can attach to it too. */
    if (CONFIG_TILTGUARD_SDA_GPIO == CONFIG_TILTGUARD_SCL_GPIO ||
        !GPIO_IS_VALID_OUTPUT_GPIO(CONFIG_TILTGUARD_SDA_GPIO) ||
        !GPIO_IS_VALID_OUTPUT_GPIO(CONFIG_TILTGUARD_SCL_GPIO)) {
        ESP_LOGE(TAG, "Invalid I2C GPIO configuration");
        return;
    }
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = CONFIG_TILTGUARD_SDA_GPIO,
        .scl_io_num = CONFIG_TILTGUARD_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        /* External pull-ups to 3.3 V are required for reliable 400 kHz operation. */
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus = NULL;
    esp_err_t err = i2c_new_master_bus(&bus_config, &bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus initialization failed: %s", esp_err_to_name(err));
        return;
    }

    const mpu6050_config_t config = {
        .address = CONFIG_TILTGUARD_MPU_ADDRESS,
        .scl_speed_hz = CONFIG_TILTGUARD_I2C_HZ,
        .timeout_ms = CONFIG_TILTGUARD_I2C_TIMEOUT_MS,
    };
    mpu6050_handle_t sensor = NULL;
    err = mpu6050_init(bus, &config, &sensor);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MPU6050 device attachment failed: %s", esp_err_to_name(err));
        esp_err_t cleanup_err = i2c_del_master_bus(bus);
        if (cleanup_err != ESP_OK) {
            ESP_LOGE(TAG, "I2C bus cleanup failed: %s", esp_err_to_name(cleanup_err));
        }
        return;
    }
    ESP_LOGI(TAG, "Phase 2: SDA=%d SCL=%d, address=0x%02x, clock=%d Hz",
             CONFIG_TILTGUARD_SDA_GPIO, CONFIG_TILTGUARD_SCL_GPIO,
             CONFIG_TILTGUARD_MPU_ADDRESS, CONFIG_TILTGUARD_I2C_HZ);

    /* app_main already runs in an ESP-IDF FreeRTOS task. A one-second identity
     * check allows reconnect diagnostics without claiming 100 Hz acquisition.
     * This task exclusively owns the device, so no additional mutex is needed.
     */
    bool connected = false;
    unsigned failures = 0;
    for (;;) {
        uint8_t identity = 0;
        err = mpu6050_check_connection(sensor, &identity);
        if (err == ESP_OK) {
            if (!connected) {
                ESP_LOGI(TAG, "Device detected at address 0x%02x", config.address);
                ESP_LOGI(TAG, "WHO_AM_I = 0x%02x", identity);
                ESP_LOGI(TAG, "Identity verified; measurement configuration is Phase 3");
            }
            connected = true;
            failures = 0;
        } else {
            /* Limit application logs to first failure and every tenth retry.
             * The ESP-IDF peripheral driver may also log its own bus errors.
             */
            if (failures == 0) {
                if (err == ESP_ERR_INVALID_RESPONSE) {
                    ESP_LOGE(TAG, "Unexpected WHO_AM_I=0x%02x (expected 0x68)", identity);
                } else {
                    ESP_LOGE(TAG, "Identity read failed: %s; check power, wiring and pull-ups",
                             esp_err_to_name(err));
                }
            }
            failures = (failures + 1) % 10;
            if (connected) {
                ESP_LOGW(TAG, "Sensor connection lost; retrying once per second");
            }
            connected = false;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
