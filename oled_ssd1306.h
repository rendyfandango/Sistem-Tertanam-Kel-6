#pragma once
#include "driver/i2c.h" // Library I2C bawaan ESP-IDF

// --- PENGATURAN PIN & I2C ---
#define I2C_MASTER_SDA_IO   21      // Ubah jika SDA OLED kamu terhubung ke pin lain
#define I2C_MASTER_SCL_IO   22      // Ubah jika SCL OLED kamu terhubung ke pin lain
#define I2C_MASTER_NUM      0      // Port I2C yang digunakan
#define I2C_MASTER_FREQ_HZ  400000 // Kecepatan I2C (400kHz)
#define OLED_I2C_ADDRESS    0x3C   // Alamat I2C default untuk OLED SSD1306

// --- DEKLARASI FUNGSI ---
void i2c_master_init(void);
void ssd1306_init(void);
void ssd1306_clear(void);
void ssd1306_draw_text(uint8_t page, uint8_t col, const char *text);