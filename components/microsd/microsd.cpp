#include "microsd.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

static const char *TAG = "MICROSD";

static sdmmc_card_t *sd_card = nullptr;
static spi_host_device_t sd_spi_host;
static bool spi_initialized = false;
static bool sd_mounted = false;


// =====================================================
// INITIALIZE MICROSD
// =====================================================

esp_err_t microsd_init(const microsd_config_t *config)
{
    if (config == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    if (sd_mounted) {
        ESP_LOGW(TAG, "MicroSD already mounted");
        return ESP_OK;
    }

    sd_spi_host = config->host;

    // SPI bus configuration
    spi_bus_config_t bus_config = {};

    bus_config.mosi_io_num = config->pin_mosi;
    bus_config.miso_io_num = config->pin_miso;
    bus_config.sclk_io_num = config->pin_sck;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;

    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(
        sd_spi_host,
        &bus_config,
        SPI_DMA_CH_AUTO
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI initialization failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    spi_initialized = true;

    // SDSPI host configuration
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = sd_spi_host;

    // SDSPI device configuration
    sdspi_device_config_t slot_config =
        SDSPI_DEVICE_CONFIG_DEFAULT();

    slot_config.gpio_cs = (gpio_num_t)config->pin_cs;
    slot_config.host_id = sd_spi_host;

    // FAT filesystem configuration
    esp_vfs_fat_mount_config_t mount_config = {};

    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 5;
    mount_config.allocation_unit_size = 16 * 1024;

    // Mount MicroSD
    ret = esp_vfs_fat_sdspi_mount(
        MICROSD_MOUNT_POINT,
        &host,
        &slot_config,
        &mount_config,
        &sd_card
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MicroSD mount failed: %s",
                 esp_err_to_name(ret));

        spi_bus_free(sd_spi_host);
        spi_initialized = false;

        return ret;
    }

    sd_mounted = true;

    ESP_LOGI(TAG, "MicroSD initialized successfully");

    sdmmc_card_print_info(stdout, sd_card);

    return ESP_OK;
}


// =====================================================
// WRITE FILE
// =====================================================

esp_err_t microsd_write_file(
    const char *filename,
    const char *data)
{
    if (!sd_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (filename == nullptr || data == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    FILE *file = fopen(filename, "w");

    if (file == nullptr) {
        ESP_LOGE(TAG, "Cannot open file: %s", filename);
        return ESP_FAIL;
    }

    size_t data_length = strlen(data);

    size_t written = fwrite(
        data,
        1,
        data_length,
        file
    );

    int close_result = fclose(file);

    if (written != data_length || close_result != 0) {
        ESP_LOGE(TAG, "File write failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "File written: %s", filename);

    return ESP_OK;
}


// =====================================================
// APPEND LINE
// =====================================================

esp_err_t microsd_append_line(
    const char *filename,
    const char *data)
{
    if (!sd_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (filename == nullptr || data == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    FILE *file = fopen(filename, "a");

    if (file == nullptr) {
        ESP_LOGE(TAG, "Cannot open file: %s", filename);
        return ESP_FAIL;
    }

    int result = fprintf(file, "%s\n", data);

    int close_result = fclose(file);

    if (result < 0 || close_result != 0) {
        ESP_LOGE(TAG, "Append failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Data appended: %s", filename);

    return ESP_OK;
}


// =====================================================
// READ FILE
// =====================================================

esp_err_t microsd_read_file(
    const char *filename,
    char *buffer,
    size_t buffer_size)
{
    if (!sd_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    if (filename == nullptr ||
        buffer == nullptr ||
        buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    FILE *file = fopen(filename, "r");

    if (file == nullptr) {
        ESP_LOGE(TAG, "Cannot open file: %s", filename);
        return ESP_FAIL;
    }

    size_t bytes_read = fread(
        buffer,
        1,
        buffer_size - 1,
        file
    );

    bool read_error = ferror(file);

    fclose(file);

    if (read_error) {
        return ESP_FAIL;
    }

    buffer[bytes_read] = '\0';

    ESP_LOGI(TAG, "File read: %s", filename);

    return ESP_OK;
}


// =====================================================
// UNMOUNT MICROSD
// =====================================================

esp_err_t microsd_unmount(void)
{
    if (!sd_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(
        MICROSD_MOUNT_POINT,
        sd_card
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Unmount failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    sd_card = nullptr;
    sd_mounted = false;

    if (spi_initialized) {
        ret = spi_bus_free(sd_spi_host);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI bus free failed");
            return ret;
        }

        spi_initialized = false;
    }

    ESP_LOGI(TAG, "MicroSD unmounted successfully");

    return ESP_OK;
}