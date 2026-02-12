## How To Use

1. Using the `USB 2.0` port, connect your CrowPanel 10.1 to your computer via USB-C while holding the `BOOT` button on the back-right of the device. This will put the device into bootloader mode, allowing you to flash new firmware.
2. Flash your device with the MicroPython firmware in the `builds/MicroPython` directory (you can use the `tools/flash.sh` script for this but make sure to update the paths in the script first).
3. Using Thonny IDE or any other serial terminal, connect to the device's serial port at `115200` baud to access the MicroPython REPL and start programming your CrowPanel 10.1 in Python! Copy the `main.py` file to the root of the device's filesystem or `main-simple.py` for a simpler demo.
