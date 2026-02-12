"""
Demo for Crow Panel Advanced 10.1-inch ESP32-P4 HMI AI Display
Boot sequence + interactive touch paint canvas
Copyright © 2026 JBlanked
https://github.com/jblanked
"""

import lcd
import touch
import time
import math
import _thread


class LCD(lcd.LCD):
    pass


class Touch(touch.Touch):
    pass


# ─── Palette ───────────────────────────────────────────────────
C_BG = 0x0841  # near-black blue
C_PANEL = 0x1082  # dark slate
C_ACCENT = 0x07FF  # cyan
C_ACCENT2 = 0xF81F  # magenta
C_GOLD = 0xFEA0  # warm gold
C_WHITE = 0xFFFF  # white
C_DIM = 0x4208  # dim grey
C_RED = 0xF800
C_GREEN = 0x07E0
C_BLUE = 0x001F
C_ORANGE = 0xFBE0  # bright orange

# ─── Screen geometry ───────────────────────────────────────────
W = 1024  # LCD_WIDTH
H = 600  # LCD_HEIGHT
CX = W // 2  # 512
CY = H // 2  # 300

# Global LCD and Touch instances
display = None
touch_sensor = None


def lerp(a, b, t):
    """Linear interpolation between a and b"""
    return a + (b - a) * t


def anim_boot():
    """Animation 1 — Boot splash with expanding concentric rings"""
    # Step 1: black wipe → background colour
    display.fill(C_BG)
    display.swap()
    time.sleep_ms(100)

    # Step 2: expanding rings pulse outward from centre
    for step in range(8):
        display.fill(C_BG)

        for r in range(step + 1):
            radius = r * 40 + 10
            # Alternate cyan / magenta rings
            col = C_ACCENT if r % 2 == 0 else C_ACCENT2
            display.circle(CX, CY, radius, col)
            # Slightly smaller filled circle for glow effect
            if radius > 4:
                display.circle(CX, CY, radius - 2, col)

        # Centre logo placeholder — filled circle + text
        display.fill_circle(CX, CY, 35, C_PANEL)
        display.circle(CX, CY, 36, C_GOLD)
        display.set_font(lcd.FONT_MEDIUM)
        display.text(CX - 20, CY - 7, "ESP32", C_GOLD)

        # Progress bar at the bottom
        bar_x = CX - 200
        bar_y = H - 60
        bar_w = 400
        bar_h = 10
        display.rect(bar_x, bar_y, bar_w, bar_h, C_DIM)
        display.fill_rect(
            bar_x + 1, bar_y + 1, ((step + 1) * bar_w) // 8 - 2, bar_h - 2, C_ACCENT
        )

        # Boot status text
        display.set_font(lcd.FONT_SMALL)
        status = f"Initialising... {(step + 1) * 12}%"
        display.text(CX - 80, bar_y + 18, status, C_DIM)

        display.swap()
        time.sleep_ms(120)

    # Step 3: flash white, then settle
    display.fill(C_WHITE)
    display.swap()
    time.sleep_ms(60)
    display.fill(C_BG)
    display.swap()
    time.sleep_ms(80)


def anim_radar_sweep():
    """Animation 2 — Radar sweep with rotating line and blips"""
    RADAR_R = 220  # outer radius of radar
    RADAR_STEPS = 72  # 72 steps = 5° per frame → full circle

    # Fixed "blip" positions (x offset, y offset from radar centre)
    blips = [(80, -60), (-120, 40), (50, 130), (-70, -140), (170, -20), (10, 90)]

    for frame in range(RADAR_STEPS + 4):
        display.fill(C_BG)

        # ── Radar scope background ────────────────────────────
        # Four concentric range rings
        for r in range(1, 5):
            display.circle(CX, CY, (RADAR_R * r) // 4, C_DIM)

        # Cross-hairs
        display.line(CX - RADAR_R, CY, CX + RADAR_R, CY, C_DIM)
        display.line(CX, CY - RADAR_R, CX, CY + RADAR_R, C_DIM)

        # Outer bezel ring (two thick circles)
        display.circle(CX, CY, RADAR_R, C_ACCENT)
        display.circle(CX, CY, RADAR_R + 2, C_ACCENT)

        # ── Sweep line ────────────────────────────────────────
        angle_rad = (frame % RADAR_STEPS) * (2.0 * math.pi / RADAR_STEPS)
        tip_x = int(CX + RADAR_R * math.cos(angle_rad))
        tip_y = int(CY + RADAR_R * math.sin(angle_rad))
        display.line(CX, CY, tip_x, tip_y, C_ACCENT)

        # Draw a slightly dimmer trailing line 10° behind
        trail_rad = angle_rad - (2.0 * math.pi / RADAR_STEPS) * 3
        tx = int(CX + RADAR_R * math.cos(trail_rad))
        ty = int(CY + RADAR_R * math.sin(trail_rad))
        display.line(CX, CY, tx, ty, C_DIM)

        # ── Centre dot ────────────────────────────────────────
        display.fill_circle(CX, CY, 4, C_ACCENT)

        # ── Blips ─────────────────────────────────────────────
        for bx_off, by_off in blips:
            bx = CX + bx_off
            by = CY + by_off
            display.fill_circle(bx, by, 5, C_GOLD)
            display.circle(bx, by, 8, C_ORANGE)

        # ── Title ─────────────────────────────────────────────
        display.set_font(lcd.FONT_LARGE)
        display.text(20, 20, "RADAR", C_ACCENT)
        display.set_font(lcd.FONT_SMALL)
        display.text(20, 55, "Target acquisition", C_DIM)

        display.swap()
        time.sleep_ms(35)


def anim_particle_burst():
    """Animation 3 — Particle burst with triangle fan"""
    BURST_STEPS = 20
    NUM_TRIS = 18

    for frame in range(BURST_STEPS):
        display.fill(C_BG)

        t = frame / (BURST_STEPS - 1)  # 0 → 1
        radius = lerp(0, 260.0, t)

        for i in range(NUM_TRIS):
            base_angle = i * (2.0 * math.pi / NUM_TRIS)
            spread = 0.18  # half-angle of triangle tip

            # Tip of the triangle
            tip_r = radius
            tip_x = int(CX + tip_r * math.cos(base_angle))
            tip_y = int(CY + tip_r * math.sin(base_angle))

            # Left & right base corners (close to origin, slightly spread)
            base_r = radius * 0.4
            bx1 = int(CX + base_r * math.cos(base_angle - spread))
            by1 = int(CY + base_r * math.sin(base_angle - spread))
            bx2 = int(CX + base_r * math.cos(base_angle + spread))
            by2 = int(CY + base_r * math.sin(base_angle + spread))

            # Cycle colours around the fan
            colors = [C_ACCENT, C_ACCENT2, C_GOLD, C_GREEN, C_ORANGE, C_WHITE]
            col = colors[i % 6]

            display.fill_triangle(tip_x, tip_y, bx1, by1, bx2, by2, col)

        # Bright core
        core_r = int(20.0 * (1.0 - t) + 4)
        display.fill_circle(CX, CY, core_r, C_WHITE)

        # Scatter tiny pixels as spark debris
        import random

        random.seed(frame * 37 + 13)
        for p in range(120):
            ang = random.random() * 2.0 * math.pi
            dist = random.random() * radius * 1.1
            px = int(CX + dist * math.cos(ang))
            py = int(CY + dist * math.sin(ang))
            if 0 <= px < W and 0 <= py < H:
                display.pixel(px, py, C_WHITE)

        display.swap()
        time.sleep_ms(40)

    # Brief pause before dashboard
    time.sleep_ms(300)


def scene_dashboard():
    """Static dashboard with HMI panel"""
    display.fill(C_BG)

    # ── Top header bar ────────────────────────────────────────
    display.fill_rect(0, 0, W, 50, C_PANEL)
    display.line(0, 50, W, 50, C_ACCENT)

    display.set_font(lcd.FONT_LARGE)
    display.text(20, 12, "CrowPanel HMI", C_WHITE)

    display.set_font(lcd.FONT_SMALL)
    display.text(W - 200, 18, "ESP32-P4  |  1024x600", C_DIM)

    # ── Left card — System gauges ─────────────────────────────
    card_x, card_y, card_w, card_h = 20, 70, 300, 480
    display.rect(card_x, card_y, card_w, card_h, C_ACCENT)
    display.fill_rect(card_x + 1, card_y + 1, card_w - 2, 32, C_PANEL)

    display.set_font(lcd.FONT_MEDIUM)
    display.text(card_x + 10, card_y + 8, "SYSTEM", C_ACCENT)

    # CPU gauge — filled rect as bar
    gauges = [
        ("CPU", 72, C_ACCENT),
        ("MEM", 45, C_GREEN),
        ("GPU", 88, C_ACCENT2),
        ("TEMP", 60, C_ORANGE),
    ]

    for g, (label, pct, col) in enumerate(gauges):
        gy = card_y + 50 + g * 60
        display.set_font(lcd.FONT_SMALL)
        display.text(card_x + 10, gy, label, C_WHITE)

        pct_str = f"{pct}%"
        display.text(card_x + card_w - 45, gy, pct_str, col)

        # Bar background
        bx = card_x + 10
        bw = card_w - 20
        display.fill_rect(bx, gy + 16, bw, 12, C_DIM)
        # Bar fill
        display.fill_rect(bx, gy + 16, (bw * pct) // 100, 12, col)
        # Bar outline
        display.rect(bx, gy + 16, bw, 12, C_PANEL)

    # Icon circles in the card footer
    for i in range(4):
        colors = [C_ACCENT, C_GREEN, C_ACCENT2, C_GOLD]
        col = colors[i]
        display.circle(card_x + 40 + i * 60, card_y + card_h - 40, 18, col)
        display.fill_circle(card_x + 40 + i * 60, card_y + card_h - 40, 8, col)

    # ── Centre card — Clock / Big number display ──────────────
    cc_x, cc_y, cc_w, cc_h = 340, 70, 344, 230
    display.fill_rect(cc_x, cc_y, cc_w, cc_h, C_PANEL)
    display.rect(cc_x, cc_y, cc_w, cc_h, C_GOLD)
    display.rect(cc_x + 2, cc_y + 2, cc_w - 4, cc_h - 4, C_DIM)  # double border

    display.set_font(lcd.FONT_LARGE)
    display.text(cc_x + 60, cc_y + 20, "12:34:56", C_GOLD)

    display.set_font(lcd.FONT_SMALL)
    display.text(cc_x + 20, cc_y + 80, "TUE  10 FEB 2026", C_DIM)

    display.line(cc_x + 10, cc_y + 100, cc_x + cc_w - 10, cc_y + 100, C_DIM)

    display.set_font(lcd.FONT_MEDIUM)
    display.text(cc_x + 30, cc_y + 115, "Uptime: 00:01:23", C_WHITE)

    # Mini trend line (fake data → polyline via individual segments)
    trend = [0, 8, 3, 15, 10, 20, 14, 25, 18, 22, 30]
    lx0 = cc_x + 20
    ly0 = cc_y + cc_h - 20
    for p in range(10):
        x1 = lx0 + p * 30
        y1 = ly0 - trend[p]
        x2 = lx0 + (p + 1) * 30
        y2 = ly0 - trend[p + 1]
        display.line(x1, y1, x2, y2, C_ACCENT)
        display.fill_circle(x1, y1, 3, C_ACCENT2)

    # ── Centre card — Diagnostics triangle ornament ───────────
    dc_x, dc_y, dc_w, dc_h = 340, 320, 344, 230
    display.fill_rect(dc_x, dc_y, dc_w, dc_h, C_PANEL)
    display.rect(dc_x, dc_y, dc_w, dc_h, C_ACCENT2)

    display.set_font(lcd.FONT_MEDIUM)
    display.text(dc_x + 10, dc_y + 10, "DIAG", C_ACCENT2)

    # Nested filled triangles — warning / status icons
    tx = dc_x + dc_w // 2
    ty = dc_y + dc_h // 2 + 10
    display.fill_triangle(tx, ty - 70, tx - 60, ty + 40, tx + 60, ty + 40, C_ORANGE)
    display.fill_triangle(tx, ty - 45, tx - 38, ty + 28, tx + 38, ty + 28, C_PANEL)
    display.fill_triangle(tx, ty - 25, tx - 20, ty + 15, tx + 20, ty + 15, C_GOLD)

    display.set_font(lcd.FONT_SMALL)
    display.text(dc_x + 10, dc_y + dc_h - 30, "All systems nominal", C_GREEN)

    # ── Right card — Wireless / connection panel ──────────────
    rc_x, rc_y, rc_w, rc_h = 704, 70, 300, 480
    display.rect(rc_x, rc_y, rc_w, rc_h, C_ACCENT)
    display.fill_rect(rc_x + 1, rc_y + 1, rc_w - 2, 32, C_PANEL)

    display.set_font(lcd.FONT_MEDIUM)
    display.text(rc_x + 10, rc_y + 8, "NETWORK", C_ACCENT)

    # Wi-Fi arc stack
    wcx = rc_x + rc_w // 2
    wcy = rc_y + 150
    for arc in [3, 2, 1]:
        colors = [C_GREEN, C_GOLD, C_ACCENT]
        col = colors[3 - arc]
        display.circle(wcx, wcy, arc * 30, col)
    display.fill_circle(wcx, wcy, 6, C_WHITE)

    display.set_font(lcd.FONT_SMALL)
    display.text(rc_x + 20, wcy + 100, "SSID: CrowNet-5G", C_WHITE)
    display.text(rc_x + 20, wcy + 120, "IP  : 192.168.1.42", C_DIM)
    display.text(rc_x + 20, wcy + 140, "RSSI: -48 dBm", C_GREEN)

    display.line(rc_x + 10, wcy + 165, rc_x + rc_w - 10, wcy + 165, C_DIM)

    # Packet stats with small rect blocks
    pkt = [
        ("TX  128.4 KB", C_ACCENT),
        ("RX   97.1 KB", C_GREEN),
        ("ERR      0", C_DIM),
    ]
    for r, (lbl, col) in enumerate(pkt):
        ry = wcy + 180 + r * 32
        display.fill_rect(rc_x + 10, ry, 8, 18, col)
        display.set_font(lcd.FONT_SMALL)
        display.text(rc_x + 26, ry + 2, lbl, col)

    # Corner decoration — mini pixel-art dot grid
    for gx in range(6):
        for gy in range(6):
            px = rc_x + rc_w - 60 + gx * 8
            py = rc_y + rc_h - 60 + gy * 8
            col = C_ACCENT if (gx + gy) % 2 == 0 else C_DIM
            display.pixel(px, py, col)

    # ── Bottom status strip ───────────────────────────────────
    display.fill_rect(0, H - 30, W, 30, C_PANEL)
    display.line(0, H - 30, W, H - 30, C_DIM)
    display.set_font(lcd.FONT_SMALL)
    display.text(10, H - 22, "READY", C_GREEN)
    display.text(CX - 80, H - 22, "Touch screen to paint", C_DIM)
    display.text(W - 140, H - 22, "v1.0.0-demo", C_DIM)

    display.swap()
    time.sleep_ms(200)


should_run = True  # Control flag for the touch task


def touch_paint_task():
    """Interactive touch paint task"""
    global display, touch_sensor, should_run
    # Cycle through a palette of brush colours
    brush_palette = [C_WHITE, C_ACCENT, C_ACCENT2, C_GOLD, C_GREEN, C_ORANGE, C_RED]
    colour_idx = 0

    last_x = 0
    last_y = 0
    had_last = False

    while True:
        if not should_run:
            print("Touch paint task stopping...")
            break
        if not touch_sensor.read():
            time.sleep_ms(10)
            continue

        if touch_sensor.pressed:
            # Advance colour every ~50 pixels of travel to create a rainbow trail effect
            if had_last:
                dx = touch_sensor.x - last_x
                dy = touch_sensor.y - last_y
                if dx * dx + dy * dy > 50 * 50:
                    colour_idx = (colour_idx + 1) % len(brush_palette)

            col = brush_palette[colour_idx]

            # Smooth stroke: draw line segment from last point
            if had_last:
                display.line(last_x, last_y, touch_sensor.x, touch_sensor.y, col)

            # Filled circle brush tip
            display.fill_circle(touch_sensor.x, touch_sensor.y, 4, col)

            last_x = touch_sensor.x
            last_y = touch_sensor.y
            had_last = True

            display.swap()
        else:
            had_last = False

        time.sleep_ms(10)


def main():
    global display, touch_sensor

    # ── Hardware init ─────────────────────────────────────────
    display = LCD()
    print("LCD initialized")

    touch_sensor = Touch()
    print("Touch initialized")

    # ── Fade brightness in ─────────────────────────────────────
    display.brightness = 0
    display.fill(C_BG)
    display.swap()

    for b in range(0, 81, 2):
        display.brightness = b
        time.sleep_ms(15)

    # ── Animated boot sequence ────────────────────────────────
    anim_boot()
    anim_radar_sweep()
    anim_particle_burst()

    # ── Static dashboard scene ────────────────────────────────
    scene_dashboard()

    # ── Launch interactive touch-paint task ───────────────────
    _thread.start_new_thread(touch_paint_task, ())


if __name__ == "__main__":
    try:
        main()
        while True:
            time.sleep(1)  # Keep the main thread alive
    except Exception:
        should_run = False  # Signal the touch task to stop
        print("Exiting...")
