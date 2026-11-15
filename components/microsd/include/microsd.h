#ifndef MICROSD_H
#define MICROSD_H

#include <stddef.h>
#include "esp_err.h"
#include "driver/spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MICROSD_MOUNT_POINT "/sdcard"

typedef struct {
    spi_host_device_t host;
    int pin_mosi;
    int pin_miso;
    int pin_sck;
    int pin_cs;
} microsd_config_t;

/**
 * @brief Initialize and mount MicroSD card.
 */
esp_err_t microsd_init(const microsd_config_t *config);

/**
 * @brief Create or overwrite a file.
 */
esp_err_t microsd_write_file(
    const char *filename,
    const char *data
);

/**
 * @brief Append a line to an existing file.
 */
esp_err_t microsd_append_line(
    const char *filename,
    const char *data
);

/**
 * @brief Read file content.
 */
esp_err_t microsd_read_file(
    const char *filename,
    char *buffer,
    size_t buffer_size
);

/**
 * @brief Unmount MicroSD card.
 */
esp_err_t microsd_unmount(void);

#ifdef __cplusplus
}
#endif

#endif