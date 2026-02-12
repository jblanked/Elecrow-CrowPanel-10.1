#!/bin/bash
# Flash CrowPanel 10.1 ESP32-P4 MicroPython Firmware
esp_idf_dir="/Users/user/.espressif/v5.5.2/esp-idf"
crowpanel_build_dir="/Users/user/esp/Elecrow-CrowPanel-10.1/builds/MicroPython"
if [ ! -d "$esp_idf_dir" ]; then
    echo "Error: ESP-IDF directory not found at $esp_idf_dir"
    exit 1
fi
arch -x86_64 bash -c "source $esp_idf_dir/export.sh && cd $crowpanel_build_dir && python -m esptool --chip esp32p4 -b 460800 --before default_reset --after hard_reset write_flash --erase-all --flash_mode dio --flash_size 16MB --flash_freq 40m 0x2000 bootloader.bin 0x8000 partition-table.bin 0x10000 Crowpanel-10.1.bin"