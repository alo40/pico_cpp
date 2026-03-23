#ifndef SSD1306_H
#define SSD1306_H

#include "hardware/i2c.h"

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_ADDR 0x3C

void ssd1306_init(i2c_inst_t *i2c);
void ssd1306_clear(void);
void ssd1306_draw_pixel(int x, int y, bool color);
void ssd1306_draw_string(int x, int y, const char *text);
void ssd1306_show(void);
void ssd1306_draw_char(int x, int y, char c);
void ssd1306_draw_string(int x, int y, const char *str);

#endif
