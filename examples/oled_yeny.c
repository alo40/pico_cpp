#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "ssd1306.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

//////////////////////////////////////////////////////
// OLED 1: Big scrolling text
//////////////////////////////////////////////////////

#define I2C_PORT_OLED_1 i2c0
#define SDA_PIN_OLED_1 0
#define SCL_PIN_OLED_1 1

//////////////////////////////////////////////////////
// OLED 2: Moving minion-style drawing
//////////////////////////////////////////////////////

#define I2C_PORT_OLED_2 i2c1
#define SDA_PIN_OLED_2 2
#define SCL_PIN_OLED_2 3

//////////////////////////////////////////////////////
// Display settings
//////////////////////////////////////////////////////

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define BIG_TEXT_SCALE 4
#define BIG_TEXT_Y 18
#define BIG_TEXT_SPEED 4

//////////////////////////////////////////////////////
// Drawing helpers
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
// Moving minion-style drawing
//////////////////////////////////////////////////////

static void draw_minion_style(int x_offset, int frame)
{
    int ox = x_offset;

    //////////////////////////////////////////////////////
    // Frame-dependent movement
    //////////////////////////////////////////////////////

    int body_y = 2;
    int eye_y = 22;

    if (frame == 1)
    {
        body_y = 4;
        eye_y = 24;
    }
    else if (frame == 2)
    {
        body_y = 1;
        eye_y = 21;
    }

    //////////////////////////////////////////////////////
    // Body
    //////////////////////////////////////////////////////

    oled_fill_round_rect(32 + ox, body_y, 64, 60, 16, true);

    //////////////////////////////////////////////////////
    // Hair
    //////////////////////////////////////////////////////

    oled_draw_line(58 + ox, body_y + 2, 54 + ox, body_y - 2, true);
    oled_draw_line(64 + ox, body_y + 2, 64 + ox, body_y - 3, true);
    oled_draw_line(70 + ox, body_y + 2, 74 + ox, body_y - 2, true);

    //////////////////////////////////////////////////////
    // Arms: 3 animation frames
    //////////////////////////////////////////////////////

    if (frame == 0)
    {
        // Both arms down
        oled_draw_line(34 + ox, body_y + 34, 20 + ox, body_y + 44, true);
        oled_draw_line(94 + ox, body_y + 34, 108 + ox, body_y + 44, true);

        oled_fill_circle(18 + ox, body_y + 45, 3, true);
        oled_fill_circle(110 + ox, body_y + 45, 3, true);
    }
    else if (frame == 1)
    {
        // Left arm up, right arm down
        oled_draw_line(34 + ox, body_y + 34, 20 + ox, body_y + 24, true);
        oled_draw_line(94 + ox, body_y + 34, 108 + ox, body_y + 44, true);

        oled_fill_circle(18 + ox, body_y + 23, 3, true);
        oled_fill_circle(110 + ox, body_y + 45, 3, true);
    }
    else
    {
        // Left arm down, right arm up
        oled_draw_line(34 + ox, body_y + 34, 20 + ox, body_y + 44, true);
        oled_draw_line(94 + ox, body_y + 34, 108 + ox, body_y + 24, true);

        oled_fill_circle(18 + ox, body_y + 45, 3, true);
        oled_fill_circle(110 + ox, body_y + 23, 3, true);
    }

    //////////////////////////////////////////////////////
    // Goggle strap
    //////////////////////////////////////////////////////

    oled_fill_rect(34 + ox, eye_y - 3, 60, 7, false);

    //////////////////////////////////////////////////////
    // One big goggle
    //////////////////////////////////////////////////////

    oled_fill_circle(64 + ox, eye_y, 15, true);
    oled_fill_circle(64 + ox, eye_y, 10, false);
    oled_fill_circle(64 + ox, eye_y, 5, true);

    //////////////////////////////////////////////////////
    // Moving pupil
    //////////////////////////////////////////////////////

    if (frame == 0)
    {
        oled_fill_circle(66 + ox, eye_y - 1, 2, false);
    }
    else if (frame == 1)
    {
        oled_fill_circle(62 + ox, eye_y, 2, false);
    }
    else
    {
        oled_fill_circle(68 + ox, eye_y, 2, false);
    }

    //////////////////////////////////////////////////////
    // Mouth
    //////////////////////////////////////////////////////

    if (frame == 0)
    {
        oled_draw_line(54 + ox, body_y + 39, 74 + ox, body_y + 39, false);
    }
    else if (frame == 1)
    {
        oled_draw_line(56 + ox, body_y + 40, 72 + ox, body_y + 42, false);
    }
    else
    {
        oled_draw_line(56 + ox, body_y + 42, 72 + ox, body_y + 40, false);
    }

    //////////////////////////////////////////////////////
    // Overalls
    //////////////////////////////////////////////////////

    oled_draw_line(48 + ox, body_y + 44, 58 + ox, body_y + 53, false);
    oled_draw_line(80 + ox, body_y + 44, 70 + ox, body_y + 53, false);

    oled_fill_rect(52 + ox, body_y + 52, 24, 9, false);
    oled_fill_rect(60 + ox, body_y + 55, 8, 4, true);

    //////////////////////////////////////////////////////
    // Legs
    //////////////////////////////////////////////////////

    oled_fill_rect(54 + ox, body_y + 61, 8, 3, true);
    oled_fill_rect(66 + ox, body_y + 61, 8, 3, true);
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
    // Animation variables
    //////////////////////////////////////////////////////

    const char big_message[] = "HOLA YENY, MUCHAS GRACIAS!";

    int big_text_width = (int)strlen(big_message) * 6 * BIG_TEXT_SCALE;

    // Start completely outside the right side
    int big_text_x = SCREEN_WIDTH;

    int minion_x = 0;
    int minion_direction = 1;
    int minion_frame = 0;

    bool led_state = false;

    //////////////////////////////////////////////////////
    // Main loop
    //////////////////////////////////////////////////////

    while (true)
    {
        //////////////////////////////////////////////////////
        // OLED 1: Big scrolling text, right to left
        //////////////////////////////////////////////////////

        ssd1306_set_i2c_port(I2C_PORT_OLED_1);

        ssd1306_clear();

        ssd1306_draw_string_scaled(
            big_text_x,
            BIG_TEXT_Y,
            big_message,
            BIG_TEXT_SCALE
        );

        ssd1306_show();

        //////////////////////////////////////////////////////
        // OLED 2: Moving minion-style animation
        //////////////////////////////////////////////////////

        ssd1306_set_i2c_port(I2C_PORT_OLED_2);

        ssd1306_clear();

        draw_minion_style(minion_x, minion_frame);

        ssd1306_show();

        //////////////////////////////////////////////////////
        // Update big text position: right to left
        //////////////////////////////////////////////////////

        big_text_x -= BIG_TEXT_SPEED;

        // When the complete text leaves the left side,
        // restart from the right side.
        if (big_text_x < -big_text_width)
        {
            big_text_x = SCREEN_WIDTH;
        }

        //////////////////////////////////////////////////////
        // Update minion movement
        //////////////////////////////////////////////////////

        minion_x += minion_direction;

        if (minion_x > 10)
        {
            minion_direction = -1;
        }

        if (minion_x < -10)
        {
            minion_direction = 1;
        }

        //////////////////////////////////////////////////////
        // Update minion animation frame
        //////////////////////////////////////////////////////

        minion_frame++;

        if (minion_frame >= 3)
        {
            minion_frame = 0;
        }

        //////////////////////////////////////////////////////
        // Blink onboard LED
        //////////////////////////////////////////////////////

        led_state = !led_state;
        gpio_put(LED_PIN, led_state);

        sleep_ms(80);
    }
}