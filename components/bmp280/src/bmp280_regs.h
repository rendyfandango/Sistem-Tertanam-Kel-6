#pragma once

#define BMP280_CHIP_ID_SAMPLE_1  0x56
#define BMP280_CHIP_ID_SAMPLE_2  0x57
#define BMP280_CHIP_ID_BMP280    0x58
#define BMP280_CHIP_ID_BME280    0x60

#define BMP280_REG_CALIB         0x88   /* 24 byte kalibrasi: 0x88..0x9F */
#define BMP280_CALIB_LEN         24
#define BMP280_REG_ID            0xD0
#define BMP280_REG_RESET         0xE0
#define BMP280_REG_STATUS        0xF3
#define BMP280_REG_CTRL_MEAS     0xF4
#define BMP280_REG_CONFIG        0xF5
#define BMP280_REG_PRESS_MSB     0xF7   /* 6 byte: press[3], temp[3] */

#define BMP280_RESET_CMD         0xB6
#define BMP280_STATUS_MEASURING  (1 << 3)
#define BMP280_STATUS_IM_UPDATE  (1 << 0)