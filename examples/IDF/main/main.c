/*
 * Demo for Crow Panel Advanced 10.1-inch ESP32-P4 HMI AI Display
 * Boot sequence + interactive touch paint canvas
 * Copyright © 2026 JBlanked
 * https://github.com/jblanked
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lcd.h"
#include "colors.h"
#include "touch.h"
#include "sd.h"

/* ─── Palette ─────────────────────────────────────────────────── */
#define C_BG 0x0841      // near-black blue
#define C_PANEL 0x1082   // dark slate
#define C_ACCENT 0x07FF  // cyan
#define C_ACCENT2 0xF81F // magenta
#define C_GOLD 0xFEA0    // warm gold
#define C_WHITE 0xFFFF   // white
#define C_DIM 0x4208     // dim grey
#define C_RED 0xF800
#define C_GREEN 0x07E0
#define C_BLUE 0x001F
#define C_ORANGE 0xFBE0 // bright orange

/* ─── Screen geometry ─────────────────────────────────────────── */
#define W LCD_WIDTH  // 1024
#define H LCD_HEIGHT //  600
#define CX (W / 2)   //  512
#define CY (H / 2)   //  300

/* ─── Helpers ─────────────────────────────────────────────────── */
#define DELAY(ms) vTaskDelay(pdMS_TO_TICKS(ms))

static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

/* ─── Forward declarations ────────────────────────────────────── */
static void anim_boot(void);
static void anim_radar_sweep(void);
static void anim_particle_burst(void);
static void scene_dashboard(void);
static void touch_paint_task(void *param);

/* ══════════════════════════════════════════════════════════════════
 *  app_main
 * ════════════════════════════════════════════════════════════════ */
void app_main(void)
{
    /* ── Hardware init ─────────────────────────────────────────── */
    if (!lcd_init())
    {
        printf("LCD init failed\n");
        return;
    }
    if (!touch_init())
    {
        printf("Touch init failed\n");
        return;
    }
    if (!sd_init())
    {
        printf("SD init failed\n");
        return;
    }

    /* ── SD card self-test ─────────────────────────────────────── */
    const char *sd_msg = "CrowPanel boot OK";
    sd_write("/sdcard/boot_log.txt", sd_msg, strlen(sd_msg), "w");

    char sd_buf[64];
    size_t n = sd_read("/sdcard/boot_log.txt", sd_buf, sizeof(sd_buf) - 1);
    if (n > 0)
    {
        sd_buf[n] = '\0';
        printf("SD: %s\n", sd_buf);
    }

    /* ── Fade backlight in ─────────────────────────────────────── */
    lcd_set_backlight(0);
    lcd_fill(C_BG);
    lcd_swap();

    for (int b = 0; b <= 80; b += 2)
    {
        lcd_set_backlight(b);
        DELAY(15);
    }

    /* ── Animated boot sequence ────────────────────────────────── */
    anim_boot();
    anim_radar_sweep();
    anim_particle_burst();

    /* ── Static dashboard scene (uses every drawing primitive) ─── */
    scene_dashboard();

    /* ── Launch interactive touch-paint task ───────────────────── */
    xTaskCreate(touch_paint_task, "touch_paint", 4096, NULL, 5, NULL);
}

/* ══════════════════════════════════════════════════════════════════
 *  ANIMATION 1 — Boot splash
 *  Draws expanding concentric rings + a logo + progress bar
 * ════════════════════════════════════════════════════════════════ */
static void anim_boot(void)
{
    /* Step 1: black wipe → background colour ──────────────────── */
    lcd_fill(C_BG);
    lcd_swap();
    DELAY(100);

    /* Step 2: expanding rings pulse outward from centre ───────── */
    for (int step = 0; step < 8; step++)
    {
        lcd_fill(C_BG);

        for (int r = 0; r <= step; r++)
        {
            uint16_t radius = (uint16_t)(r * 40 + 10);
            // Alternate cyan / magenta rings
            uint16_t col = (r % 2 == 0) ? C_ACCENT : C_ACCENT2;
            lcd_draw_circle(CX, CY, radius, col);
            // Slightly smaller filled circle for glow effect
            if (radius > 4)
                lcd_draw_circle(CX, CY, radius - 2, col);
        }

        /* Centre logo placeholder — filled circle + text */
        lcd_fill_circle(CX, CY, 35, C_PANEL);
        lcd_draw_circle(CX, CY, 36, C_GOLD);
        lcd_set_font(FONT_MEDIUM);
        lcd_draw_text(CX - 20, CY - 7, "ESP32", C_GOLD);

        /* Progress bar at the bottom */
        uint16_t bar_x = CX - 200;
        uint16_t bar_y = H - 60;
        uint16_t bar_w = 400;
        uint16_t bar_h = 10;
        lcd_draw_rect(bar_x, bar_y, bar_w, bar_h, C_DIM);
        lcd_fill_rect(bar_x + 1, bar_y + 1,
                      (uint16_t)(((step + 1) * bar_w) / 8 - 2),
                      bar_h - 2, C_ACCENT);

        /* Boot status text */
        lcd_set_font(FONT_SMALL);
        char status[40];
        snprintf(status, sizeof(status), "Initialising... %d%%", (step + 1) * 12);
        lcd_draw_text(CX - 80, bar_y + 18, status, C_DIM);

        lcd_swap();
        DELAY(120);
    }

    /* Step 3: flash white, then settle */
    lcd_fill(C_WHITE);
    lcd_swap();
    DELAY(60);
    lcd_fill(C_BG);
    lcd_swap();
    DELAY(80);
}

/* ══════════════════════════════════════════════════════════════════
 *  ANIMATION 2 — Radar sweep
 *  Rotating line draws a classic radar/sonar sweep with fading dots
 * ════════════════════════════════════════════════════════════════ */
static void anim_radar_sweep(void)
{
#define RADAR_R 220    // outer radius of radar
#define RADAR_STEPS 72 // 72 steps = 5° per frame → full circle

    /* Fixed "blip" positions (x offset, y offset from radar centre) */
    static const int16_t blips[][2] = {
        {80, -60}, {-120, 40}, {50, 130}, {-70, -140}, {170, -20}, {10, 90}};
    const int nb = sizeof(blips) / sizeof(blips[0]);

    for (int frame = 0; frame < RADAR_STEPS + 4; frame++)
    {
        lcd_fill(C_BG);

        /* ── Radar scope background ─────────────────────────────── */
        // Four concentric range rings
        for (int r = 1; r <= 4; r++)
            lcd_draw_circle(CX, CY, (uint16_t)(RADAR_R * r / 4), C_DIM);

        // Cross-hairs
        lcd_draw_line(CX - RADAR_R, CY, CX + RADAR_R, CY, C_DIM);
        lcd_draw_line(CX, CY - RADAR_R, CX, CY + RADAR_R, C_DIM);

        // Outer bezel ring (two thick circles)
        lcd_draw_circle(CX, CY, RADAR_R, C_ACCENT);
        lcd_draw_circle(CX, CY, RADAR_R + 2, C_ACCENT);

        /* ── Sweep line ─────────────────────────────────────────── */
        float angle_rad = (float)(frame % RADAR_STEPS) * (2.0f * 3.14159f / RADAR_STEPS);
        int16_t tip_x = (int16_t)(CX + RADAR_R * cosf(angle_rad));
        int16_t tip_y = (int16_t)(CY + RADAR_R * sinf(angle_rad));
        lcd_draw_line(CX, CY, (uint16_t)tip_x, (uint16_t)tip_y, C_ACCENT);

        /* Draw a slightly dimmer trailing line 10° behind */
        float trail_rad = angle_rad - (2.0f * 3.14159f / RADAR_STEPS) * 3;
        int16_t tx = (int16_t)(CX + RADAR_R * cosf(trail_rad));
        int16_t ty = (int16_t)(CY + RADAR_R * sinf(trail_rad));
        lcd_draw_line(CX, CY, (uint16_t)tx, (uint16_t)ty, C_DIM);

        /* ── Centre dot ─────────────────────────────────────────── */
        lcd_fill_circle(CX, CY, 4, C_ACCENT);

        /* ── Blips ──────────────────────────────────────────────── */
        for (int b = 0; b < nb; b++)
        {
            uint16_t bx = (uint16_t)(CX + blips[b][0]);
            uint16_t by = (uint16_t)(CY + blips[b][1]);
            lcd_fill_circle(bx, by, 5, C_GOLD);
            lcd_draw_circle(bx, by, 8, C_ORANGE);
        }

        /* ── Title ──────────────────────────────────────────────── */
        lcd_set_font(FONT_LARGE);
        lcd_draw_text(20, 20, "RADAR", C_ACCENT);
        lcd_set_font(FONT_SMALL);
        lcd_draw_text(20, 55, "Target acquisition", C_DIM);

        lcd_swap();
        DELAY(35);
    }
}

/* ══════════════════════════════════════════════════════════════════
 *  ANIMATION 3 — Particle burst + triangle fan
 *  Draws filled triangles radiating outward like an explosion
 * ════════════════════════════════════════════════════════════════ */
static void anim_particle_burst(void)
{
#define BURST_STEPS 20
#define NUM_TRIS 18

    for (int frame = 0; frame < BURST_STEPS; frame++)
    {
        lcd_fill(C_BG);

        float t = (float)frame / (float)(BURST_STEPS - 1); // 0 → 1
        float radius = lerp(0, 260.0f, t);
        float alpha = 1.0f - t; // used to fade colour (crude: pick from palette)

        for (int i = 0; i < NUM_TRIS; i++)
        {
            float base_angle = (float)i * (2.0f * 3.14159f / NUM_TRIS);
            float spread = 0.18f; // half-angle of triangle tip

            /* Tip of the triangle */
            float tip_r = radius;
            uint16_t tip_x = (uint16_t)(CX + tip_r * cosf(base_angle));
            uint16_t tip_y = (uint16_t)(CY + tip_r * sinf(base_angle));

            /* Left & right base corners (close to origin, slightly spread) */
            float base_r = radius * 0.4f;
            uint16_t bx1 = (uint16_t)(CX + base_r * cosf(base_angle - spread));
            uint16_t by1 = (uint16_t)(CY + base_r * sinf(base_angle - spread));
            uint16_t bx2 = (uint16_t)(CX + base_r * cosf(base_angle + spread));
            uint16_t by2 = (uint16_t)(CY + base_r * sinf(base_angle + spread));

            /* Cycle colours around the fan */
            uint16_t col;
            switch (i % 6)
            {
            case 0:
                col = C_ACCENT;
                break;
            case 1:
                col = C_ACCENT2;
                break;
            case 2:
                col = C_GOLD;
                break;
            case 3:
                col = C_GREEN;
                break;
            case 4:
                col = C_ORANGE;
                break;
            default:
                col = C_WHITE;
                break;
            }
            lcd_fill_triangle(tip_x, tip_y, bx1, by1, bx2, by2, col);
        }

        /* Bright core */
        uint16_t core_r = (uint16_t)(20.0f * (1.0f - t) + 4);
        lcd_fill_circle(CX, CY, core_r, C_WHITE);

        /* Scatter tiny pixels as spark debris */
        srand(frame * 37 + 13);
        for (int p = 0; p < 120; p++)
        {
            float ang = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;
            float dist = ((float)rand() / RAND_MAX) * radius * 1.1f;
            uint16_t px = (uint16_t)(CX + dist * cosf(ang));
            uint16_t py = (uint16_t)(CY + dist * sinf(ang));
            if (px < W && py < H)
                lcd_draw_pixel(px, py, C_WHITE);
        }

        lcd_swap();
        DELAY(40);
    }

    /* Brief pause before dashboard */
    DELAY(300);
}

/* ══════════════════════════════════════════════════════════════════
 *  SCENE — Static dashboard
 *  A polished HMI panel that uses every remaining primitive
 * ════════════════════════════════════════════════════════════════ */
static void scene_dashboard(void)
{
    lcd_fill(C_BG);

    /* ── Top header bar ─────────────────────────────────────────── */
    lcd_fill_rect(0, 0, W, 50, C_PANEL);
    lcd_draw_line(0, 50, W, 50, C_ACCENT);

    lcd_set_font(FONT_LARGE);
    lcd_draw_text(20, 12, "CrowPanel HMI", C_WHITE);

    lcd_set_font(FONT_SMALL);
    lcd_draw_text(W - 200, 18, "ESP32-P4  |  1024x600", C_DIM);

    /* ── Left card — System gauges ──────────────────────────────── */
    uint16_t card_x = 20, card_y = 70, card_w = 300, card_h = 480;
    lcd_draw_rect(card_x, card_y, card_w, card_h, C_ACCENT);
    lcd_fill_rect(card_x + 1, card_y + 1, card_w - 2, 32, C_PANEL);

    lcd_set_font(FONT_MEDIUM);
    lcd_draw_text(card_x + 10, card_y + 8, "SYSTEM", C_ACCENT);

    /* CPU gauge — filled rect as bar */
    struct
    {
        const char *label;
        uint16_t pct;
        uint16_t col;
    } gauges[] = {
        {"CPU", 72, C_ACCENT},
        {"MEM", 45, C_GREEN},
        {"GPU", 88, C_ACCENT2},
        {"TEMP", 60, C_ORANGE},
    };
    for (int g = 0; g < 4; g++)
    {
        uint16_t gy = card_y + 50 + g * 60;
        lcd_set_font(FONT_SMALL);
        lcd_draw_text(card_x + 10, gy, gauges[g].label, C_WHITE);

        char pct_str[8];
        snprintf(pct_str, sizeof(pct_str), "%d%%", gauges[g].pct);
        lcd_draw_text(card_x + card_w - 45, gy, pct_str, gauges[g].col);

        /* Bar background */
        uint16_t bx = card_x + 10, bw = card_w - 20;
        lcd_fill_rect(bx, gy + 16, bw, 12, C_DIM);
        /* Bar fill */
        lcd_fill_rect(bx, gy + 16, (uint16_t)(bw * gauges[g].pct / 100), 12, gauges[g].col);
        /* Bar outline */
        lcd_draw_rect(bx, gy + 16, bw, 12, C_PANEL);
    }

    /* Icon circles in the card footer */
    for (int i = 0; i < 4; i++)
    {
        uint16_t col = (i == 0) ? C_ACCENT : (i == 1) ? C_GREEN
                                         : (i == 2)   ? C_ACCENT2
                                                      : C_GOLD;
        lcd_draw_circle(card_x + 40 + i * 60, card_y + card_h - 40, 18, col);
        lcd_fill_circle(card_x + 40 + i * 60, card_y + card_h - 40, 8, col);
    }

    /* ── Centre card — Clock / Big number display ───────────────── */
    uint16_t cc_x = 340, cc_y = 70, cc_w = 344, cc_h = 230;
    lcd_fill_rect(cc_x, cc_y, cc_w, cc_h, C_PANEL);
    lcd_draw_rect(cc_x, cc_y, cc_w, cc_h, C_GOLD);
    lcd_draw_rect(cc_x + 2, cc_y + 2, cc_w - 4, cc_h - 4, C_DIM); // double border

    lcd_set_font(FONT_LARGE);
    lcd_draw_text(cc_x + 60, cc_y + 20, "12:34:56", C_GOLD);

    lcd_set_font(FONT_SMALL);
    lcd_draw_text(cc_x + 20, cc_y + 80, "TUE  10 FEB 2026", C_DIM);

    lcd_draw_line(cc_x + 10, cc_y + 100, cc_x + cc_w - 10, cc_y + 100, C_DIM);

    lcd_set_font(FONT_MEDIUM);
    lcd_draw_text(cc_x + 30, cc_y + 115, "Uptime: 00:01:23", C_WHITE);

    /* Mini trend line (fake data → polyline via individual segments) */
    static const int16_t trend[] = {0, 8, 3, 15, 10, 20, 14, 25, 18, 22, 30};
    uint16_t lx0 = cc_x + 20, ly0 = cc_y + cc_h - 20;
    for (int p = 0; p < 10; p++)
    {
        uint16_t x1 = lx0 + p * 30;
        uint16_t y1 = ly0 - trend[p];
        uint16_t x2 = lx0 + (p + 1) * 30;
        uint16_t y2 = ly0 - trend[p + 1];
        lcd_draw_line(x1, y1, x2, y2, C_ACCENT);
        lcd_fill_circle(x1, y1, 3, C_ACCENT2);
    }

    /* ── Centre card — Diagnostics triangle ornament ────────────── */
    uint16_t dc_x = 340, dc_y = 320, dc_w = 344, dc_h = 230;
    lcd_fill_rect(dc_x, dc_y, dc_w, dc_h, C_PANEL);
    lcd_draw_rect(dc_x, dc_y, dc_w, dc_h, C_ACCENT2);

    lcd_set_font(FONT_MEDIUM);
    lcd_draw_text(dc_x + 10, dc_y + 10, "DIAG", C_ACCENT2);

    /* Nested filled triangles — warning / status icons */
    uint16_t tx = dc_x + dc_w / 2, ty = dc_y + dc_h / 2 + 10;
    lcd_fill_triangle(tx, ty - 70, tx - 60, ty + 40, tx + 60, ty + 40, C_ORANGE);
    lcd_fill_triangle(tx, ty - 45, tx - 38, ty + 28, tx + 38, ty + 28, C_PANEL);
    lcd_fill_triangle(tx, ty - 25, tx - 20, ty + 15, tx + 20, ty + 15, C_GOLD);

    lcd_set_font(FONT_SMALL);
    lcd_draw_text(dc_x + 10, dc_y + dc_h - 30, "All systems nominal", C_GREEN);

    /* ── Right card — Wireless / connection panel ───────────────── */
    uint16_t rc_x = 704, rc_y = 70, rc_w = 300, rc_h = 480;
    lcd_draw_rect(rc_x, rc_y, rc_w, rc_h, C_ACCENT);
    lcd_fill_rect(rc_x + 1, rc_y + 1, rc_w - 2, 32, C_PANEL);

    lcd_set_font(FONT_MEDIUM);
    lcd_draw_text(rc_x + 10, rc_y + 8, "NETWORK", C_ACCENT);

    /* Wi-Fi arc stack (draw_circle arcs approximated with full circles + overdraw) */
    uint16_t wcx = rc_x + rc_w / 2, wcy = rc_y + 150;
    for (int arc = 3; arc >= 1; arc--)
    {
        uint16_t col = (arc == 3) ? C_ACCENT : (arc == 2) ? C_GOLD
                                                          : C_GREEN;
        lcd_draw_circle(wcx, wcy, (uint16_t)(arc * 30), col);
    }
    lcd_fill_circle(wcx, wcy, 6, C_WHITE);

    lcd_set_font(FONT_SMALL);
    lcd_draw_text(rc_x + 20, wcy + 100, "SSID: CrowNet-5G", C_WHITE);
    lcd_draw_text(rc_x + 20, wcy + 120, "IP  : 192.168.1.42", C_DIM);
    lcd_draw_text(rc_x + 20, wcy + 140, "RSSI: -48 dBm", C_GREEN);

    lcd_draw_line(rc_x + 10, wcy + 165, rc_x + rc_w - 10, wcy + 165, C_DIM);

    /* Packet stats with small rect blocks */
    struct
    {
        const char *lbl;
        uint16_t col;
    } pkt[] = {
        {"TX  128.4 KB", C_ACCENT},
        {"RX   97.1 KB", C_GREEN},
        {"ERR      0", C_DIM},
    };
    for (int r = 0; r < 3; r++)
    {
        uint16_t ry = wcy + 180 + r * 32;
        lcd_fill_rect(rc_x + 10, ry, 8, 18, pkt[r].col);
        lcd_set_font(FONT_SMALL);
        lcd_draw_text(rc_x + 26, ry + 2, pkt[r].lbl, pkt[r].col);
    }

    /* Corner decoration — mini pixel-art dot grid */
    for (int gx = 0; gx < 6; gx++)
        for (int gy = 0; gy < 6; gy++)
            lcd_draw_pixel((uint16_t)(rc_x + rc_w - 60 + gx * 8),
                           (uint16_t)(rc_y + rc_h - 60 + gy * 8),
                           ((gx + gy) % 2 == 0) ? C_ACCENT : C_DIM);

    /* ── Bottom status strip ─────────────────────────────────────── */
    lcd_fill_rect(0, H - 30, W, 30, C_PANEL);
    lcd_draw_line(0, H - 30, W, H - 30, C_DIM);
    lcd_set_font(FONT_SMALL);
    lcd_draw_text(10, H - 22, "READY", C_GREEN);
    lcd_draw_text(CX - 80, H - 22, "Touch screen to paint", C_DIM);
    lcd_draw_text(W - 140, H - 22, "v1.0.0-demo", C_DIM);

    lcd_swap();
    DELAY(200);
}

/* ══════════════════════════════════════════════════════════════════
 *  TASK — Interactive touch paint
 *  Lets the user finger-draw on top of the dashboard.
 *  Draws a brush circle at each touch point and a line from
 *  the last known position for smooth strokes.
 * ════════════════════════════════════════════════════════════════ */
static void touch_paint_task(void *param)
{
    /* Cycle through a palette of brush colours */
    static const uint16_t brush_palette[] = {
        C_WHITE, C_ACCENT, C_ACCENT2, C_GOLD, C_GREEN, C_ORANGE, C_RED};
    const int num_colours = sizeof(brush_palette) / sizeof(brush_palette[0]);
    int colour_idx = 0;

    uint16_t last_x = 0, last_y = 0;
    bool had_last = false;

    while (1)
    {
        if (!touch_read())
        {
            printf("touch_paint: read error\n");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        TouchPoint pt = touch_get_point();

        if (pt.pressed)
        {
            /* Advance colour every ~50 pixels of travel to create
             * a rainbow trail effect */
            if (had_last)
            {
                int dx = (int)pt.x - (int)last_x;
                int dy = (int)pt.y - (int)last_y;
                if (dx * dx + dy * dy > 50 * 50)
                    colour_idx = (colour_idx + 1) % num_colours;
            }

            uint16_t col = brush_palette[colour_idx];

            /* Smooth stroke: draw line segment from last point */
            if (had_last)
                lcd_draw_line(last_x, last_y, pt.x, pt.y, col);

            /* Filled circle brush tip */
            lcd_fill_circle(pt.x, pt.y, 4, col);

            last_x = pt.x;
            last_y = pt.y;
            had_last = true;

            lcd_swap();
        }
        else
        {
            had_last = false;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}