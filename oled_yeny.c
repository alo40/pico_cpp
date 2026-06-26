#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "ssd1306.h"
#include <stdbool.h>
#include <stdio.h>

//////////////////////////////////////////////////////
// OLED 1: Big text display
//////////////////////////////////////////////////////

#define I2C_PORT_OLED_1 i2c0
#define SDA_PIN_OLED_1 0
#define SCL_PIN_OLED_1 1

//////////////////////////////////////////////////////
// OLED 2: Minion-style drawing
//////////////////////////////////////////////////////

#define I2C_PORT_OLED_2 i2c1
#define SDA_PIN_OLED_2 2
#define SCL_PIN_OLED_2 3

//////////////////////////////////////////////////////
// Simple drawing helpers
//////////////////////////////////////////////////////

static int iabs_int(int v)
{
    return v < 0 ? -v : v;
}

static void oled_fill_rect(int x, int y, int w, int h, bool color)
{
    for (int dx = 0; dx < w; dx++)
    {
        for (int dy = 0; dy < h; dy++)
        {
            ssd1306_draw_pixel(x + dx, y + dy, color);
        }
    }
}

static void oled_draw_line(int x0, int y0, int x1, int y1, bool color)
{
    int dx = iabs_int(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;

    int dy = -iabs_int(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while (true)
    {
        ssd1306_draw_pixel(x0, y0, color);

        if (x0 == x1 && y0 == y1)
        {
            break;
        }

        int e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

static void oled_fill_circle(int cx, int cy, int r, bool color)
{
    for (int y = -r; y <= r; y++)
    {
        for (int x = -r; x <= r; x++)
        {
            if ((x * x) + (y * y) <= r * r)
            {
                ssd1306_draw_pixel(cx + x, cy + y, color);
            }
        }
    }
}

static void oled_fill_round_rect(int x, int y, int w, int h, int r, bool color)
{
    for (int py = 0; py < h; py++)
    {
        for (int px = 0; px < w; px++)
        {
            int dx = 0;
            int dy = 0;

            if (px < r)
            {
                dx = r - px;
            }
            else if (px >= w - r)
            {
                dx = px - (w - r - 1);
            }

            if (py < r)
            {
                dy = r - py;
            }
            else if (py >= h - r)
            {
                dy = py - (h - r - 1);
            }

            if ((dx * dx) + (dy * dy) <= r * r)
            {
                ssd1306_draw_pixel(x + px, y + py, color);
            }
        }
    }
}

//////////////////////////////////////////////////////
// Minion-style monochrome drawing
//////////////////////////////////////////////////////

static void draw_minion_style(void)
{
    //////////////////////////////////////////////////////
    // Body
    //////////////////////////////////////////////////////

    oled_fill_round_rect(32, 2, 64, 60, 16, true);

    //////////////////////////////////////////////////////
    // Hair
    //////////////////////////////////////////////////////

    oled_draw_line(58, 4, 54, 0, true);
    oled_draw_line(64, 4, 64, 0, true);
    oled_draw_line(70, 4, 74, 0, true);

    //////////////////////////////////////////////////////
    // Arms
    //////////////////////////////////////////////////////

    oled_draw_line(34, 34, 20, 44, true);
    oled_draw_line(94, 34, 108, 44, true);

    oled_fill_circle(18, 45, 3, true);
    oled_fill_circle(110, 45, 3, true);

    //////////////////////////////////////////////////////
    // Goggle strap
    // false clears pixels, making black details
    //////////////////////////////////////////////////////

    oled_fill_rect(34, 19, 60, 7, false);

    //////////////////////////////////////////////////////
    // One big goggle
    //////////////////////////////////////////////////////

    oled_fill_circle(64, 22, 15, true);   // white outer ring
    oled_fill_circle(64, 22, 10, false);  // black inside
    oled_fill_circle(64, 22, 5, true);    // white eye
    oled_fill_circle(66, 21, 2, false);   // black pupil

    //////////////////////////////////////////////////////
    // Mouth
    //////////////////////////////////////////////////////

    oled_draw_line(54, 39, 74, 39, false);
    oled_draw_line(56, 40, 72, 42, false);

    //////////////////////////////////////////////////////
    // Overalls
    //////////////////////////////////////////////////////

    oled_draw_line(48, 44, 58, 53, false);
    oled_draw_line(80, 44, 70, 53, false);

    oled_fill_rect(52, 52, 24, 9, false);

    // Small pocket
    oled_fill_rect(60, 55, 8, 4, true);

    //////////////////////////////////////////////////////
    // Legs
    //////////////////////////////////////////////////////

    oled_fill_rect(54, 61, 8, 3, true);
    oled_fill_rect(66, 61, 8, 3, true);
}

int main()
{
    stdio_init_all();

    //////////////////////////////////////////////////////
    // Onboard LED
    //////////////////////////////////////////////////////

    const uint LED_PIN = 25;

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    //////////////////////////////////////////////////////
    // I2C setup for OLED 1
    //////////////////////////////////////////////////////

    i2c_init(I2C_PORT_OLED_1, 400000);

    gpio_set_function(SDA_PIN_OLED_1, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN_OLED_1, GPIO_FUNC_I2C);

    gpio_pull_up(SDA_PIN_OLED_1);
    gpio_pull_up(SCL_PIN_OLED_1);

    //////////////////////////////////////////////////////
    // I2C setup for OLED 2
    //////////////////////////////////////////////////////

    i2c_init(I2C_PORT_OLED_2, 400000);

    gpio_set_function(SDA_PIN_OLED_2, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN_OLED_2, GPIO_FUNC_I2C);

    gpio_pull_up(SDA_PIN_OLED_2);
    gpio_pull_up(SCL_PIN_OLED_2);

    //////////////////////////////////////////////////////
    // OLED initialization
    //////////////////////////////////////////////////////

    ssd1306_init(I2C_PORT_OLED_1);
    ssd1306_clear();
    ssd1306_show();

    ssd1306_init(I2C_PORT_OLED_2);
    ssd1306_clear();
    ssd1306_show();

    //////////////////////////////////////////////////////
    // Main loop
    //////////////////////////////////////////////////////

    bool led_state = false;

    while (true)
    {
        //////////////////////////////////////////////////////
        // OLED 1: Super big text
        //////////////////////////////////////////////////////

        ssd1306_set_i2c_port(I2C_PORT_OLED_1);

        ssd1306_clear();

        // Scale 6 gives very large text.
        // "HI!" fits nicely on 128x64.
        ssd1306_draw_string_scaled(10, 10, "HOLA YENY!", 6);

        // Smaller subtitle
        ssd1306_draw_string(28, 58, "OLED 1");

        ssd1306_show();

        //////////////////////////////////////////////////////
        // OLED 2: Minion-style drawing
        //////////////////////////////////////////////////////

        ssd1306_set_i2c_port(I2C_PORT_OLED_2);

        ssd1306_clear();

        draw_minion_style();

        ssd1306_show();

        //////////////////////////////////////////////////////
        // Blink onboard LED
        //////////////////////////////////////////////////////

        led_state = !led_state;
        gpio_put(LED_PIN, led_state);

        sleep_ms(500);
    }
}