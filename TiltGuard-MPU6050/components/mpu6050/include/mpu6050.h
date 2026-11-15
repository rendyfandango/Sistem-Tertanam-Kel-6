#ifndef TILTGUARD_MPU6050_H
#define TILTGUARD_MPU6050_H

#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

/* Opaque pointer: callers cannot alter private driver state. */
typedef struct mpu6050_device *mpu6050_handle_t;

typedef struct {
    uint8_t address;          /* Unshifted 7-bit address: 0x68 or 0x69. */
    uint32_t scl_speed_hz;    /* 10000..400000 Hz. */
    int timeout_ms;           /* Positive, bounded synchronous transaction wait. */
} mpu6050_config_t;

#define MPU6050_CONFIG_DEFAULT() { .address = 0x68, .scl_speed_hz = 400000, .timeout_ms = 100 }

/* Phase 2: attach a device only; no reset, wake, or measurement configuration.
 * bus remains owned by the application. *out must initially be NULL.
 * Success here does not prove hardware presence: call check_connection next.
 * Serialize calls/lifetime externally; no asynchronous callbacks are registered.
 */
esp_err_t mpu6050_init(i2c_master_bus_handle_t bus,
                       const mpu6050_config_t *config, mpu6050_handle_t *out);

/* Read full WHO_AM_I byte. Output changes only after a successful I2C read,
 * including an unexpected ID. ESP_ERR_INVALID_RESPONSE means ID != 0x68.
 * WHO_AM_I is 0x68 even when AD0 selects slave address 0x69.
 */
esp_err_t mpu6050_check_connection(mpu6050_handle_t device, uint8_t *who_am_i);

/* Remove only this device, never the external bus. On success set *device=NULL.
 * On removal failure retain the handle so the caller can retry safely.
 */
esp_err_t mpu6050_deinit(mpu6050_handle_t *device);

#endif /* TILTGUARD_MPU6050_H */
