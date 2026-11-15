# Source this file in bash or zsh. Uses the existing EIM installation read-only.
# This is a machine-specific helper, not a replacement for installing ESP-IDF.
export IDF_PATH=/home/sayed/.espressif/v6.1/esp-idf
export IDF_TOOLS_PATH=/home/sayed/.espressif/tools
export IDF_PYTHON_ENV_PATH=/home/sayed/.espressif/tools/python/v6.1/venv
export ESP_IDF_VERSION=6.1
export IDF_VERSION=6.1.0
export ESP_ROM_ELF_DIR=/home/sayed/.espressif/tools/esp-rom-elfs/20241011
export PATH="$IDF_PYTHON_ENV_PATH/bin:/home/sayed/.espressif/tools/xtensa-esp-elf/esp-15.2.0_20251204/xtensa-esp-elf/bin:/home/sayed/.espressif/tools/esp32ulp-elf/2.38_20240113/esp32ulp-elf/bin:$IDF_PATH/tools:$PATH"
