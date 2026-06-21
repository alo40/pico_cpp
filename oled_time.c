#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "ssd1306.h"
#include <stdio.h>

#define I2C_PORT i2c0
#define SDA_PIN 0
#define SCL_PIN 1

void draw_circle(int cx, int cy, int radius)
{
    for (int y = -radius; y <= radius; y++)
    {
        for (int x = -radius; x <= radius; x++)
        {
            if (x * x + y * y <= radius * radius)
            {
                ssd1306_draw_pixel(cx + x, cy + y, true);
            }
        }
    }
}

int main()
{
    stdio_init_all();

    //////////////////////////////////////////////////////

    // The onboard LED is usually GPIO 25 (Pico)
    const uint LED_PIN = 25;

    // Initialize the LED pin
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    /* while (true) { */
    /*     gpio_put(LED_PIN, 1); // LED ON */
    /*     sleep_ms(500); */

    /*     gpio_put(LED_PIN, 0); // LED OFF */
    /*     sleep_ms(500); */
    /* } */

    //////////////////////////////////////////////////////

    i2c_init(I2C_PORT, 400000);

    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    ssd1306_init(I2C_PORT);
    ssd1306_clear();

    /* // Draw straight line */
    /* for (int i = 0; i < 64; i++) { */
    /*     ssd1306_draw_pixel(i, i, true); */
    /* } */

    int i = 0;              // position offset
    int vel = 4;            // speed and direction
    int screen_width = 128; // OLED width
    int cx = screen_width / 2;
    int cy = 32;
    int radius = 8; // circle radius

    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    uint32_t last_time_ms = to_ms_since_boot(get_absolute_time());

    int text_x = 0;
    int text_speed = 1;
    const char message[] = "Hello Pico!";
    int text_width = 11 * 6; // 11 chars, approx. 6 px per char

    while (true)
    {
        uint32_t now_ms = to_ms_since_boot(get_absolute_time());

        if (now_ms - last_time_ms >= 1000)
        { // 1 second elapsed
            last_time_ms = now_ms;

            // increment time
            seconds++;
            if (seconds >= 60)
            {
                seconds = 0;
                minutes++;
                if (minutes >= 60)
                {
                    minutes = 0;
                    hours++;
                    if (hours >= 24)
                        hours = 0;
                }
            }
        }

        ssd1306_clear();

        // Display time as HH:MM:SS
        char time_str[9]; // "HH:MM:SS"
        sprintf(time_str, "%02d:%02d:%02d", hours, minutes, seconds);
        ssd1306_draw_string(10, 0, time_str);

        // Optional: keep your circle animation
        draw_circle(cx - i, cy, radius);
        // draw_circle(cx + i, cy, radius);

        // Moving message at the lower part of the OLED
        ssd1306_draw_string(text_x, 56, message);

        ssd1306_show();

        // Move text left to right
        text_x += text_speed;

        // If text leaves the right side, restart from the left
        if (text_x > screen_width)
        {
            text_x = -text_width;
        }

        // move circle
        if (cx + i + radius + vel >= screen_width ||
            cx + i + radius + vel <= 10)
            vel = -vel;
        i += vel;

        sleep_ms(10); // small delay to reduce CPU usage
    }
}
