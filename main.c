#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oled_ssd1306.h"

void app_main(void)
{
    // 1. Inisialisasi I2C & OLED
    i2c_master_init();
    ssd1306_init();
    
    // 2. Wajib bersihkan memori acak sebelum menulis
    ssd1306_clear();
    
    // 3. Tulis teks! (Baris 3, Kolom 20)
    ssd1306_draw_text(3, 20, "HALO ESP32-S3!");
    
    // Tulis teks lain di (Baris 5, Kolom 10)
    ssd1306_draw_text(5, 10, "BERHASIL NYALA :)");
    
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}