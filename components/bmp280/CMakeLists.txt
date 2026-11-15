idf_component_register(
    SRCS "src/bmp280.c" "src/bmp280_bus.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES driver
)