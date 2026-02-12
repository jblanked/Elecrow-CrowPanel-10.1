#!/bin/bash
# Script to build and install the MicroPython version of Crowpanel 10.1 ESP32-P4

echo "Initializing and preparing build environment..."

# set locations
micropython_dir="/Users/user/pico/micropython/ports/esp32"
micropython_root="/Users/user/pico/micropython"
crowpanel_dir="/Users/user/esp/Elecrow-CrowPanel-10.1"
esp_idf_dir="/Users/user/.espressif/v5.5.2/esp-idf"

DIRECTORY="$crowpanel_dir/builds/MicroPython"

if [ ! -d "$DIRECTORY" ]; then
  mkdir -p "$DIRECTORY"
fi

rm -rf "$crowpanel_dir"/builds/MicroPython/Crowpanel-10.1.bin

rm -rf "$micropython_dir"/build-ESP32_GENERIC_P4-C6_WIFI

echo "Building MicroPython Crowpanel firmware..."

# copy crowpanel modules to micropython modules directory
rm -rf "$micropython_dir"/modules/crowpanel
mkdir -p "$micropython_dir"/modules/crowpanel
cp "$crowpanel_dir"/src/MicroPython/micropython.cmake "$micropython_dir"/modules/crowpanel/micropython.cmake
cp -r "$crowpanel_dir"/src/MicroPython/lcd "$micropython_dir"/modules/crowpanel/lcd
cp -r "$crowpanel_dir"/src/MicroPython/touch "$micropython_dir"/modules/crowpanel/touch
cp -r "$crowpanel_dir"/src/MicroPython/sd "$micropython_dir"/modules/crowpanel/sd

# copy custom partitions.csv for ESP32-P4 (larger flash partitions)
cp "$crowpanel_dir"/src/MicroPython/partitions.csv "$micropython_dir"/boards/ESP32_GENERIC_P4/partitions.csv
cp "$crowpanel_dir"/src/MicroPython/partitions.csv "$micropython_dir"/partitions.csv

# copy sdkconfig.defaults with I2C conflict check skip and other essential settings
cp "$crowpanel_dir"/src/MicroPython/sdkconfig.defaults "$micropython_dir"/boards/ESP32_GENERIC_P4/sdkconfig.defaults

# copy custom mpconfigboard.cmake to use our sdkconfig.defaults
cp "$crowpanel_dir"/src/MicroPython/mpconfigboard.cmake "$micropython_dir"/boards/ESP32_GENERIC_P4/mpconfigboard.cmake

# copy custom C6_WIFI variant that includes our sdkconfig.defaults
cp "$crowpanel_dir"/src/MicroPython/mpconfigvariant_C6_WIFI.cmake "$micropython_dir"/boards/ESP32_GENERIC_P4/mpconfigvariant_C6_WIFI.cmake

# copy custom mpconfigboard.h to disable incompatible features
cp "$crowpanel_dir"/src/MicroPython/mpconfigboard.h "$micropython_dir"/boards/ESP32_GENERIC_P4/mpconfigboard.h

# Build mpy-cross
echo "Building mpy-cross with native toolchain..."
cd "$micropython_root"
make -C mpy-cross clean
make -C mpy-cross -j4
if [ $? -ne 0 ]; then
    echo "ERROR: mpy-cross build failed!"
    exit 1
fi

# Now source ESP-IDF environment
echo "Setting up ESP-IDF environment..."
source "$esp_idf_dir/export.sh"

# move to the micropython esp32 port directory
cd "$micropython_dir"

echo "Starting build process..."

# Set compiler flags to disable problematic warnings in ESP-IDF 5.5.2
# and skip I2C driver conflict check
export EXTRA_CFLAGS="-Wno-maybe-uninitialized -Wno-error=maybe-uninitialized -DCONFIG_I2C_SKIP_LEGACY_CONFLICT_CHECK=1"

# ESP32-P4 build with C6_WIFI variant for ESP-Hosted WiFi/BT via ESP32-C6
# 32MB PSRAM and 16MB Flash
make BOARD=ESP32_GENERIC_P4 BOARD_VARIANT=C6_WIFI \
     USER_C_MODULES="$micropython_dir"/modules/crowpanel/micropython.cmake \
     clean

make BOARD=ESP32_GENERIC_P4 BOARD_VARIANT=C6_WIFI \
     USER_C_MODULES="$micropython_dir"/modules/crowpanel/micropython.cmake

BUILD_DIR="$micropython_dir/build-ESP32_GENERIC_P4-C6_WIFI"
cp "$BUILD_DIR"/micropython.bin " v/Crowpanel-10.1.bin
cp "$BUILD_DIR"/bootloader/bootloader.bin "$crowpanel_dir"/builds/MicroPython/bootloader.bin
cp "$BUILD_DIR"/partition_table/partition-table.bin "$crowpanel_dir"/builds/MicroPython/partition-table.bin
cp "$BUILD_DIR"/ota_data_initial.bin "$crowpanel_dir"/builds/MicroPython/ota_data_initial.bin
echo "CrowPanel 10.1 build complete."
