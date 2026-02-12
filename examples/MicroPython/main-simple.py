"""
Demo for Crow Panel Advanced 10.1-inch ESP32-P4 HMI AI Display
Copyright © 2026 JBlanked
https://github.com/jblanked
"""

import lcd
import touch
import time
import _thread


class LCD(lcd.LCD):
    pass


class Touch(touch.Touch):
    pass


t = None  # Touch instance placeholder
l = None  # LCD instance placeholder
should_run = True  # Control flag for the touch task


def touch_task():
    """Task function: continuously reads touch data and logs coordinates"""
    global t, l, should_run

    while True:
        if not should_run:
            print("Touch task stopping...")
            break

        if t.read():
            # Draw a white pixel at the touch location
            l.pixel(t.x, t.y, lcd.COLOR_WHITE)
            l.swap()

        time.sleep_ms(10)  # Delay 10ms between reads


def main():
    global t, l
    # Initialize LCD
    l = LCD()
    print("LCD initialized")

    # Initialize touch panel
    t = Touch()
    print("Touch initialized")

    # Create a thread for reading touch data
    _thread.start_new_thread(touch_task, ())

    # Set backlight brightness to 50%
    l.brightness = 50

    # Draw a red circle
    l.circle(100, 100, 50, lcd.COLOR_RED)

    # Draw a green line
    l.line(250, 250, 200, 200, lcd.COLOR_GREEN)

    # Draw text
    l.set_font(lcd.FONT_MEDIUM)
    l.text(550, 550, "Hello, World!", lcd.COLOR_BLUE)

    # Draw a filled yellow circle
    l.fill_circle(800, 400, 30, lcd.COLOR_YELLOW)

    # Update the LCD with the new frame buffer content
    l.swap()


if __name__ == "__main__":
    try:
        main()
        while True:
            time.sleep(1)  # Keep the main thread alive
    except Exception:
        should_run = False  # Signal the touch task to stop
        print("Exiting...")
