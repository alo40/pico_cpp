#include "ssd1306.h"
#include "pico/stdlib.h"
#include <string.h>

static uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static i2c_inst_t *i2c_port;

static const uint8_t font5x7[96][5] = {
    // ASCII 32..127
    // Each char = 5 columns (LSB = top pixel)

    ['0' - 32] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ['1' - 32] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ['2' - 32] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3' - 32] = {0x21, 0x41, 0x45, 0x4B, 0x31},

    ['A' - 32] = {0x7E, 0x11, 0x11, 0x11, 0x7E},
    ['B' - 32] = {0x7F, 0x49, 0x49, 0x49, 0x36},
    ['C' - 32] = {0x3E, 0x41, 0x41, 0x41, 0x22},
};

static void write_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(i2c_port, SSD1306_ADDR, buf, 2, false);
}

void ssd1306_init(i2c_inst_t *i2c) {
    i2c_port = i2c;

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

void ssd1306_clear(void) {
    memset(buffer, 0, sizeof(buffer));
}

void ssd1306_draw_pixel(int x, int y, bool color) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;

    if (color)
        buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y % 8));
    else
        buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
}

void ssd1306_show(void) {
    for (int page = 0; page < 8; page++) {
        write_cmd(0xB0 + page);
        write_cmd(0x00);
        write_cmd(0x10);

        uint8_t data[SSD1306_WIDTH + 1];
        data[0] = 0x40;
        memcpy(&data[1], &buffer[SSD1306_WIDTH * page], SSD1306_WIDTH);

        i2c_write_blocking(i2c_port, SSD1306_ADDR, data, SSD1306_WIDTH + 1, false);
    }
}

void ssd1306_draw_char(int x, int y, char c) {
    if (c < 32 || c > 127) return;

    const uint8_t *bitmap = font5x7[c - 32];

    for (int col = 0; col < 5; col++) {
        uint8_t line = bitmap[col];

        for (int row = 0; row < 7; row++) {
            bool pixel = line & (1 << row);
            ssd1306_draw_pixel(x + col, y + row, pixel);
        }
    }
}

void ssd1306_draw_string(int x, int y, const char *str) {
    while (*str) {
        ssd1306_draw_char(x, y, *str);
        x += 6; // spacing
        str++;
    }
}
