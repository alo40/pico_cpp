#include "ssd1306.h"
#include "pico/stdlib.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static i2c_inst_t *i2c_port;

typedef enum
{
    SSD1306_BUS_HARDWARE_I2C,
    SSD1306_BUS_SOFTWARE_I2C
} ssd1306_bus_type_t;

static ssd1306_bus_type_t bus_type = SSD1306_BUS_HARDWARE_I2C;

static uint sw_sda_pin = 0;
static uint sw_scl_pin = 1;

static const uint8_t font5x7[96][5] = {
    // ASCII 32 ' '
    [0] = {0x00, 0x00, 0x00, 0x00, 0x00},  // ' '
    [1] = {0x00, 0x00, 0x5F, 0x00, 0x00},  // '!'
    [2] = {0x00, 0x07, 0x00, 0x07, 0x00},  // '"'
    [3] = {0x14, 0x7F, 0x14, 0x7F, 0x14},  // '#'
    [4] = {0x24, 0x2A, 0x7F, 0x2A, 0x12},  // '$'
    [5] = {0x23, 0x13, 0x08, 0x64, 0x62},  // '%'
    [6] = {0x36, 0x49, 0x55, 0x22, 0x50},  // '&'
    [7] = {0x00, 0x05, 0x03, 0x00, 0x00},  // '''
    [8] = {0x00, 0x1C, 0x22, 0x41, 0x00},  // '('
    [9] = {0x00, 0x41, 0x22, 0x1C, 0x00},  // ')'
    [10] = {0x14, 0x08, 0x3E, 0x08, 0x14}, // '*'
    [11] = {0x08, 0x08, 0x3E, 0x08, 0x08}, // '+'
    [12] = {0x00, 0x50, 0x30, 0x00, 0x00}, // ','
    [13] = {0x08, 0x08, 0x08, 0x08, 0x08}, // '-'
    [14] = {0x00, 0x60, 0x60, 0x00, 0x00}, // '.'
    [15] = {0x20, 0x10, 0x08, 0x04, 0x02}, // '/'
    // Digits '0'-'9' (ASCII 48–57)
    [16] = {0x3E, 0x51, 0x49, 0x45, 0x3E}, // '0'
    [17] = {0x00, 0x42, 0x7F, 0x40, 0x00}, // '1'
    [18] = {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    [19] = {0x21, 0x41, 0x45, 0x4B, 0x31}, // '3'
    [20] = {0x18, 0x14, 0x12, 0x7F, 0x10}, // '4'
    [21] = {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    [22] = {0x3C, 0x4A, 0x49, 0x49, 0x30}, // '6'
    [23] = {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    [24] = {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    [25] = {0x06, 0x49, 0x49, 0x29, 0x1E}, // '9'
    // Punctuation: ':' = ASCII 58
    [26] = {0x00, 0x11, 0x00, 0x11, 0x00}, // ':'
    // Punctuation: '<', '=', '>', '?'
    [28] = {0x08, 0x14, 0x22, 0x41, 0x00}, // '<'
    [29] = {0x14, 0x14, 0x14, 0x14, 0x14}, // '='
    [30] = {0x00, 0x41, 0x22, 0x14, 0x08}, // '>'
    [31] = {0x02, 0x01, 0x51, 0x09, 0x06}, // '?'
    // Uppercase 'A'-'Z' ASCII 65–90
    [33] = {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 'A'
    [34] = {0x7F, 0x49, 0x49, 0x49, 0x36}, // 'B'
    [35] = {0x3E, 0x41, 0x41, 0x41, 0x22}, // 'C'
    [36] = {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 'D'
    [37] = {0x7F, 0x49, 0x49, 0x49, 0x41}, // 'E'
    [38] = {0x7F, 0x09, 0x09, 0x09, 0x01}, // 'F'
    [39] = {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 'G'
    [40] = {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 'H'
    [41] = {0x00, 0x41, 0x7F, 0x41, 0x00}, // 'I'
    [42] = {0x20, 0x40, 0x41, 0x3F, 0x01}, // 'J'
    [43] = {0x7F, 0x08, 0x14, 0x22, 0x41}, // 'K'
    [44] = {0x7F, 0x40, 0x40, 0x40, 0x40}, // 'L'
    [45] = {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 'M'
    [46] = {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 'N'
    [47] = {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 'O'
    [48] = {0x7F, 0x09, 0x09, 0x09, 0x06}, // 'P'
    [49] = {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 'Q'
    [50] = {0x7F, 0x09, 0x19, 0x29, 0x46}, // 'R'
    [51] = {0x46, 0x49, 0x49, 0x49, 0x31}, // 'S'
    [52] = {0x01, 0x01, 0x7F, 0x01, 0x01}, // 'T'
    [53] = {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 'U'
    [54] = {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 'V'
    [55] = {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 'W'
    [56] = {0x63, 0x14, 0x08, 0x14, 0x63}, // 'X'
    [57] = {0x07, 0x08, 0x70, 0x08, 0x07}, // 'Y'
    [58] = {0x61, 0x51, 0x49, 0x45, 0x43}, // 'Z'
    // Lowercase 'a'-'z' ASCII 97–122
    [65] = {0x20, 0x54, 0x54, 0x54, 0x78}, // 'a'
    [66] = {0x7F, 0x48, 0x44, 0x44, 0x38}, // 'b'
    [67] = {0x38, 0x44, 0x44, 0x44, 0x20}, // 'c'
    [68] = {0x38, 0x44, 0x44, 0x48, 0x7F}, // 'd'
    [69] = {0x38, 0x54, 0x54, 0x54, 0x18}, // 'e'
    [70] = {0x08, 0x7E, 0x09, 0x01, 0x02}, // 'f'
    [71] = {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 'g'
    [72] = {0x7F, 0x08, 0x04, 0x04, 0x78}, // 'h'
    [73] = {0x00, 0x44, 0x7D, 0x40, 0x00}, // 'i'
    [74] = {0x20, 0x40, 0x44, 0x3D, 0x00}, // 'j'
    [75] = {0x7F, 0x10, 0x28, 0x44, 0x00}, // 'k'
    [76] = {0x00, 0x41, 0x7F, 0x40, 0x00}, // 'l'
    [77] = {0x7C, 0x04, 0x18, 0x04, 0x78}, // 'm'
    [78] = {0x7C, 0x08, 0x04, 0x04, 0x78}, // 'n'
    [79] = {0x38, 0x44, 0x44, 0x44, 0x38}, // 'o'
    [80] = {0x7C, 0x14, 0x14, 0x14, 0x08}, // 'p'
    [81] = {0x08, 0x14, 0x14, 0x18, 0x7C}, // 'q'
    [82] = {0x7C, 0x08, 0x04, 0x04, 0x08}, // 'r'
    [83] = {0x48, 0x54, 0x54, 0x54, 0x20}, // 's'
    [84] = {0x04, 0x3F, 0x44, 0x40, 0x20}, // 't'
    [85] = {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 'u'
    [86] = {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 'v'
    [87] = {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 'w'
    [88] = {0x44, 0x28, 0x10, 0x28, 0x44}, // 'x'
    [89] = {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 'y'
    [90] = {0x44, 0x64, 0x54, 0x4C, 0x44}, // 'z'
};
//////////////////////////////////////////////////////
// Software I2C support
//////////////////////////////////////////////////////

#define SW_I2C_DELAY_US 2

static void sw_i2c_delay(void)
{
    sleep_us(SW_I2C_DELAY_US);
}

static void sw_i2c_release(uint pin)
{
    gpio_set_dir(pin, GPIO_IN);
}

static void sw_i2c_low(uint pin)
{
    gpio_put(pin, 0);
    gpio_set_dir(pin, GPIO_OUT);
}

static void sw_i2c_prepare_pin(uint pin)
{
    gpio_init(pin);
    gpio_pull_up(pin);

    // Keep output latch low.
    // Releasing the line is done by switching the pin to input mode.
    gpio_put(pin, 0);
    gpio_set_dir(pin, GPIO_IN);
}

static void sw_i2c_start(void)
{
    sw_i2c_release(sw_sda_pin);
    sw_i2c_release(sw_scl_pin);
    sw_i2c_delay();

    sw_i2c_low(sw_sda_pin);
    sw_i2c_delay();

    sw_i2c_low(sw_scl_pin);
    sw_i2c_delay();
}

static void sw_i2c_stop(void)
{
    sw_i2c_low(sw_sda_pin);
    sw_i2c_delay();

    sw_i2c_release(sw_scl_pin);
    sw_i2c_delay();

    sw_i2c_release(sw_sda_pin);
    sw_i2c_delay();
}

static bool sw_i2c_write_byte(uint8_t byte)
{
    for (int bit = 7; bit >= 0; bit--)
    {
        if (byte & (1 << bit))
        {
            sw_i2c_release(sw_sda_pin);
        }
        else
        {
            sw_i2c_low(sw_sda_pin);
        }

        sw_i2c_delay();

        sw_i2c_release(sw_scl_pin);
        sw_i2c_delay();

        sw_i2c_low(sw_scl_pin);
        sw_i2c_delay();
    }

    // ACK bit
    sw_i2c_release(sw_sda_pin);
    sw_i2c_delay();

    sw_i2c_release(sw_scl_pin);
    sw_i2c_delay();

    bool ack = !gpio_get(sw_sda_pin);

    sw_i2c_low(sw_scl_pin);
    sw_i2c_delay();

    return ack;
}

static void sw_i2c_write_blocking(const uint8_t *data, size_t len)
{
    sw_i2c_start();

    // Address + write bit
    sw_i2c_write_byte((SSD1306_ADDR << 1) | 0);

    for (size_t i = 0; i < len; i++)
    {
        sw_i2c_write_byte(data[i]);
    }

    sw_i2c_stop();
}

//////////////////////////////////////////////////////
// Common SSD1306 write layer
//////////////////////////////////////////////////////

static void ssd1306_write_bytes(const uint8_t *data, size_t len)
{
    if (bus_type == SSD1306_BUS_HARDWARE_I2C)
    {
        i2c_write_blocking(i2c_port, SSD1306_ADDR, data, len, false);
    }
    else
    {
        sw_i2c_write_blocking(data, len);
    }
}

static void write_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};
    ssd1306_write_bytes(buf, 2);
}

static void ssd1306_init_sequence(void)
{
    sleep_ms(100);

    write_cmd(0xAE); // display off
    write_cmd(0x20); // memory mode
    write_cmd(0x00); // horizontal addressing
    write_cmd(0xB0);
    write_cmd(0xC8);
    write_cmd(0x00);
    write_cmd(0x10);
    write_cmd(0x40);
    write_cmd(0x81);
    write_cmd(0xFF);
    write_cmd(0xA1);
    write_cmd(0xA6);
    write_cmd(0xA8);
    write_cmd(0x3F);
    write_cmd(0xA4);
    write_cmd(0xD3);
    write_cmd(0x00);
    write_cmd(0xD5);
    write_cmd(0xF0);
    write_cmd(0xD9);
    write_cmd(0x22);
    write_cmd(0xDA);
    write_cmd(0x12);
    write_cmd(0xDB);
    write_cmd(0x20);
    write_cmd(0x8D);
    write_cmd(0x14);
    write_cmd(0xAF); // display ON

    memset(buffer, 0, sizeof(buffer));
}

//////////////////////////////////////////////////////
// Public API
//////////////////////////////////////////////////////

void ssd1306_init(i2c_inst_t *i2c)
{
    i2c_port = i2c;
    bus_type = SSD1306_BUS_HARDWARE_I2C;

    ssd1306_init_sequence();
}

void ssd1306_init_sw(uint sda_pin, uint scl_pin)
{
    sw_sda_pin = sda_pin;
    sw_scl_pin = scl_pin;

    sw_i2c_prepare_pin(sw_sda_pin);
    sw_i2c_prepare_pin(sw_scl_pin);

    bus_type = SSD1306_BUS_SOFTWARE_I2C;

    ssd1306_init_sequence();
}

void ssd1306_set_i2c_port(i2c_inst_t *i2c)
{
    i2c_port = i2c;
    bus_type = SSD1306_BUS_HARDWARE_I2C;
}

void ssd1306_set_sw_i2c_pins(uint sda_pin, uint scl_pin)
{
    sw_sda_pin = sda_pin;
    sw_scl_pin = scl_pin;

    bus_type = SSD1306_BUS_SOFTWARE_I2C;
}

void ssd1306_clear(void)
{
    memset(buffer, 0, sizeof(buffer));
}

void ssd1306_draw_pixel(int x, int y, bool color)
{
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT)
    {
        return;
    }

    if (color)
    {
        buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y % 8));
    }
    else
    {
        buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
    }
}

void ssd1306_show(void)
{
    for (int page = 0; page < SSD1306_HEIGHT / 8; page++)
    {
        write_cmd(0xB0 + page);
        write_cmd(0x00);
        write_cmd(0x10);

        uint8_t data[SSD1306_WIDTH + 1];
        data[0] = 0x40;

        memcpy(&data[1], &buffer[SSD1306_WIDTH * page], SSD1306_WIDTH);

        ssd1306_write_bytes(data, SSD1306_WIDTH + 1);
    }
}

void ssd1306_draw_char(int x, int y, char c)
{
    if (c < 32 || c > 126)
    {
        return;
    }

    const uint8_t *bitmap = font5x7[c - 32];

    for (int col = 0; col < 5; col++)
    {
        uint8_t line = bitmap[col];

        for (int row = 0; row < 7; row++)
        {
            bool pixel = line & (1 << row);
            ssd1306_draw_pixel(x + col, y + row, pixel);
        }
    }
}

void ssd1306_draw_string(int x, int y, const char *str)
{
    while (*str)
    {
        ssd1306_draw_char(x, y, *str);
        x += 6;
        str++;
    }
}

static void ssd1306_fill_rect(int x, int y, int w, int h, bool color)
{
    for (int dx = 0; dx < w; dx++)
    {
        for (int dy = 0; dy < h; dy++)
        {
            ssd1306_draw_pixel(x + dx, y + dy, color);
        }
    }
}

void ssd1306_draw_char_scaled(int x, int y, char c, int scale)
{
    if (scale < 1)
    {
        scale = 1;
    }

    if (c < 32 || c > 126)
    {
        return;
    }

    const uint8_t *bitmap = font5x7[c - 32];

    for (int col = 0; col < 5; col++)
    {
        uint8_t line = bitmap[col];

        for (int row = 0; row < 7; row++)
        {
            bool pixel = line & (1 << row);

            ssd1306_fill_rect(x + col * scale, y + row * scale, scale, scale,
                              pixel);
        }
    }

    // Blank spacing column
    ssd1306_fill_rect(x + 5 * scale, y, scale, 7 * scale, false);
}

void ssd1306_draw_string_scaled(int x, int y, const char *str, int scale)
{
    if (scale < 1)
    {
        scale = 1;
    }

    while (*str)
    {
        ssd1306_draw_char_scaled(x, y, *str, scale);
        x += 6 * scale;
        str++;
    }
}