#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "ssd1306.h"
#include <stdio.h>
#include <string.h>

//////////////////////////////////////////////////////
// OLED 1
//////////////////////////////////////////////////////

#define I2C_PORT_OLED_1 i2c0
#define SDA_PIN_OLED_1 0
#define SCL_PIN_OLED_1 1

//////////////////////////////////////////////////////
// OLED 2
//////////////////////////////////////////////////////

#define I2C_PORT_OLED_2 i2c1
#define SDA_PIN_OLED_2 2
#define SCL_PIN_OLED_2 3

//////////////////////////////////////////////////////
// OLED 3
// Requested physical pins 21 and 22
// Pico physical pin 21 = GPIO16
// Pico physical pin 22 = GPIO17
//////////////////////////////////////////////////////

#define SDA_PIN_OLED_3 16
#define SCL_PIN_OLED_3 17

//////////////////////////////////////////////////////
// OLED 4
// Requested physical pins 24 and 25
// Pico physical pin 24 = GPIO18
// Pico physical pin 25 = GPIO19
//////////////////////////////////////////////////////

#define SDA_PIN_OLED_4 18
#define SCL_PIN_OLED_4 19

//////////////////////////////////////////////////////
// Text animation settings
//////////////////////////////////////////////////////

#define TEXT_SCALE 2
#define TEXT_SPEED 2

static int text_width_scaled(const char *text, int scale)
{
    return strlen(text) * 6 * scale;
}

static void draw_screen_text(const char *text, int x)
{
    int y = (SSD1306_HEIGHT - (7 * TEXT_SCALE)) / 2;

    ssd1306_clear();
    ssd1306_draw_string_scaled(x, y, text, TEXT_SCALE);
    ssd1306_show();
}

int main()
{
    stdio_init_all();

    //////////////////////////////////////////////////////
    // Hardware I2C setup for OLED 1
    //////////////////////////////////////////////////////

    i2c_init(I2C_PORT_OLED_1, 400000);

    gpio_set_function(SDA_PIN_OLED_1, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN_OLED_1, GPIO_FUNC_I2C);

    gpio_pull_up(SDA_PIN_OLED_1);
    gpio_pull_up(SCL_PIN_OLED_1);

    //////////////////////////////////////////////////////
    // Hardware I2C setup for OLED 2
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

    // OLED 3 and OLED 4 use software I2C.
    // This allows independent screens even if all modules use address 0x3C.
    ssd1306_init_sw(SDA_PIN_OLED_3, SCL_PIN_OLED_3);
    ssd1306_clear();
    ssd1306_show();

    ssd1306_init_sw(SDA_PIN_OLED_4, SCL_PIN_OLED_4);
    ssd1306_clear();
    ssd1306_show();

    //////////////////////////////////////////////////////
    // Moving text state
    //////////////////////////////////////////////////////

    const char *screen_text[4] = {
        "SCREEN 1",
        "SCREEN 2",
        "SCREEN 3",
        "SCREEN 4"
    };

    int text_x[4] = {
        SSD1306_WIDTH,
        SSD1306_WIDTH,
        SSD1306_WIDTH,
        SSD1306_WIDTH
    };

    //////////////////////////////////////////////////////
    // Main loop
    //////////////////////////////////////////////////////

    while (true)
    {
        //////////////////////////////////////////////////////
        // Screen 1
        //////////////////////////////////////////////////////

        ssd1306_set_i2c_port(I2C_PORT_OLED_1);
        draw_screen_text(screen_text[0], text_x[0]);

        //////////////////////////////////////////////////////
        // Screen 2
        //////////////////////////////////////////////////////

        ssd1306_set_i2c_port(I2C_PORT_OLED_2);
        draw_screen_text(screen_text[1], text_x[1]);

        //////////////////////////////////////////////////////
        // Screen 3
        //////////////////////////////////////////////////////

        ssd1306_set_sw_i2c_pins(SDA_PIN_OLED_3, SCL_PIN_OLED_3);
        draw_screen_text(screen_text[2], text_x[2]);

        //////////////////////////////////////////////////////
        // Screen 4
        //////////////////////////////////////////////////////

        ssd1306_set_sw_i2c_pins(SDA_PIN_OLED_4, SCL_PIN_OLED_4);
        draw_screen_text(screen_text[3], text_x[3]);

        //////////////////////////////////////////////////////
        // Update right-to-left animation
        //////////////////////////////////////////////////////

        for (int i = 0; i < 4; i++)
        {
            text_x[i] -= TEXT_SPEED;

            int text_width = text_width_scaled(screen_text[i], TEXT_SCALE);

            if (text_x[i] < -text_width)
            {
                text_x[i] = SSD1306_WIDTH;
            }
        }

        sleep_ms(20);
    }
}