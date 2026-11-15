idf_component_register(
    SRCS "microsd.cpp"
    INCLUDE_DIRS "include"
    REQUIRES
        fatfs
        sdmmc
        esp_driver_spi
        esp_driver_sdspi
        driver
)