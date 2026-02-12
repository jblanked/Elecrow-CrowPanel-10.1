from machine import SDCard
import os

# For ESP32-P4/C6 CrowPanel - uses SDMMC mode
sd = SDCard(slot=1)  # Use slot 1 for SDIO

# Mount the SD card
os.mount(sd, "/sd")

# Test it
print(os.listdir("/sd"))

# When done
os.umount("/sd")
