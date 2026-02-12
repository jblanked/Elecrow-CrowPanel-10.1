/*
 * Demo for Crow Panel Advanced 10.1-inch ESP32-P4 HMI AI Display
 * Copyright © 2026 JBlanked
 * https://github.com/jblanked
 *
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h" // FreeRTOS base header
#include "freertos/task.h"     // FreeRTOS task APIs
#include "lcd.h"
#include "colors.h"
#include "touch.h"
#include "sd.h"

TaskHandle_t touch_task_handle = NULL; // Handle for the touch reading task

// Task function: continuously reads touch data and logs coordinates
void touch_task(void *param)
{
    while (1)
    {
        if (!touch_read())
        {
            printf("main: Failed to read touch data\n");
            return;
        }

        TouchPoint point = touch_get_point(); // Get the latest touch point data
        if (point.pressed)
        {
            // Draw a white pixel at the touch location
            lcd_draw_pixel(point.x, point.y, COLOR_WHITE);
            lcd_swap();
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Delay 10ms between reads
    }
}

void app_main(void)
{
    if (!lcd_init()) // Initialize LCD
    {
        printf("main: Failed to initialize LCD\n");
        return;
    }

    if (!touch_init()) // Initialize touch panel
    {
        printf("main: Failed to initialize touch panel\n");
        return;
    }

    if (!sd_init()) // Initialize SD card
    {
        printf("main: Failed to initialize SD card\n");
        return;
    }

    sd_write("/sdcard/sd_test.txt", "Hello, SD card!", 17, "w"); // Test writing to SD card

    char buffer[64];
    size_t read_size = sd_read("/sdcard/sd_test.txt", buffer, sizeof(buffer) - 1); // Test reading from SD card
    if (read_size > 0)
    {
        buffer[read_size] = '\0'; // Null-terminate the read data
        printf("Read from SD card: %s\n", buffer);
    }

    // Create a FreeRTOS task for reading touch data
    xTaskCreate(touch_task, "touch_task", 4096, NULL, 5, &touch_task_handle);

    lcd_set_backlight(50); // Set backlight brightness to 50%

    lcd_draw_circle(100, 100, 50, COLOR_RED); // Draw a red circle

    lcd_draw_line(250, 250, 200, 200, COLOR_GREEN); // Draw a green line

    lcd_draw_text(550, 550, "Hello, World!", COLOR_BLUE); // Draw text

    lcd_fill_circle(800, 400, 30, COLOR_YELLOW); // Draw a filled yellow circle

    lcd_swap(); // Update the LCD with the new frame buffer content
}
