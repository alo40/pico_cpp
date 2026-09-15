#include "dashboard.h"

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "ssd1306.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEXT_X_OFFSET 2

#define I2C_PORT_OLED_1 i2c0
#define SDA_PIN_OLED_1 0
#define SCL_PIN_OLED_1 1

#define I2C_PORT_OLED_2 i2c1
#define SDA_PIN_OLED_2 2
#define SCL_PIN_OLED_2 3

#define SDA_PIN_OLED_3 16
#define SCL_PIN_OLED_3 17

#define SDA_PIN_OLED_4 18
#define SCL_PIN_OLED_4 19

#define HISTORY_SIZE SSD1306_WIDTH
#define GRAPH_TOP 10
#define GRAPH_BOTTOM (SSD1306_HEIGHT - 1)

static vedirect_measurement_t measurement;
static long battery_history[HISTORY_SIZE];
static long panel_history[HISTORY_SIZE];
static size_t history_count;

static void add_history_sample(long battery, long panel)
{
    if (history_count < HISTORY_SIZE)
    {
        battery_history[history_count] = battery;
        panel_history[history_count++] = panel;
        return;
    }

    memmove(battery_history, battery_history + 1,
            (HISTORY_SIZE - 1) * sizeof(*battery_history));
    memmove(panel_history, panel_history + 1,
            (HISTORY_SIZE - 1) * sizeof(*panel_history));
    battery_history[HISTORY_SIZE - 1] = battery;
    panel_history[HISTORY_SIZE - 1] = panel;
}

static void draw_line(int x0, int y0, int x1, int y1)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    while (true)
    {
        ssd1306_draw_pixel(x0, y0, true);

        if (x0 == x1 && y0 == y1)
        {
            break;
        }

        int e2 = 2 * error;
        if (e2 >= dy)
        {
            error += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            error += dx;
            y0 += sy;
        }
    }
}

static void calculate_graph_range(const long *history, size_t count,
                                  long minimum_span, long *minimum,
                                  long *maximum)
{
    if (count == 0)
    {
        *minimum = 0;
        *maximum = minimum_span;
        return;
    }

    long min_value = history[0];
    long max_value = history[0];

    for (size_t i = 1; i < count; i++)
    {
        if (history[i] < min_value)
        {
            min_value = history[i];
        }
        if (history[i] > max_value)
        {
            max_value = history[i];
        }
    }

    long span = max_value - min_value;
    if (span < minimum_span)
    {
        long center = (min_value + max_value) / 2;
        min_value = center - minimum_span / 2;
        max_value = center + minimum_span / 2;
    }
    else
    {
        long margin = span / 10;
        min_value -= margin;
        max_value += margin;
    }

    if (min_value < 0)
    {
        min_value = 0;
    }
    if (max_value <= min_value)
    {
        max_value = min_value + 1;
    }

    *minimum = min_value;
    *maximum = max_value;
}

static int voltage_to_y(long value, long minimum, long maximum)
{
    if (value < minimum)
    {
        value = minimum;
    }
    if (value > maximum)
    {
        value = maximum;
    }

    return GRAPH_BOTTOM - (int)(((int64_t)(value - minimum) *
                                  (GRAPH_BOTTOM - GRAPH_TOP)) /
                                 (maximum - minimum));
}

static void draw_voltage_graph(const char *name, long current_mv,
                               const long *history, size_t count,
                               long minimum_span_mv)
{
    char title[24];

    ssd1306_clear();
    if (count == 0)
    {
        ssd1306_draw_string_scaled(TEXT_X_OFFSET, 0, name, 1);
        ssd1306_draw_string_scaled(TEXT_X_OFFSET, 20, "Waiting...", 1);
        ssd1306_show();
        return;
    }

    snprintf(title, sizeof(title), "%s %.2fV", name, current_mv / 1000.0);
    ssd1306_draw_string_scaled(TEXT_X_OFFSET, 0, title, 1);

    long graph_min;
    long graph_max;
    calculate_graph_range(history, count, minimum_span_mv, &graph_min,
                          &graph_max);

    for (int x = 0; x < SSD1306_WIDTH; x += 8)
    {
        ssd1306_draw_pixel(x, GRAPH_TOP, true);
        ssd1306_draw_pixel(x, GRAPH_BOTTOM, true);
    }

    int previous_y = voltage_to_y(history[0], graph_min, graph_max);
    ssd1306_draw_pixel(0, previous_y, true);
    for (size_t i = 1; i < count; i++)
    {
        int current_y = voltage_to_y(history[i], graph_min, graph_max);
        draw_line((int)i - 1, previous_y, (int)i, current_y);
        previous_y = current_y;
    }

    ssd1306_show();
}

static void draw_counter_screen(const vedirect_parser_t *parser)
{
    char line1[24];
    char line2[24];
    char line3[24];

    snprintf(line1, sizeof(line1), "RX: %lu",
             (unsigned long)parser->received_blocks);
    snprintf(line2, sizeof(line2), "OK: %lu",
             (unsigned long)parser->valid_blocks);
    snprintf(line3, sizeof(line3), "CK:%lu IN:%lu",
             (unsigned long)parser->invalid_checksum_blocks,
             (unsigned long)parser->incomplete_blocks);

    ssd1306_clear();
    ssd1306_draw_string_scaled(TEXT_X_OFFSET, 0, "VE.Direct", 1);
    ssd1306_draw_string_scaled(TEXT_X_OFFSET, 15, line1, 1);
    ssd1306_draw_string_scaled(TEXT_X_OFFSET, 30, line2, 1);
    ssd1306_draw_string_scaled(TEXT_X_OFFSET, 45, line3, 1);
    ssd1306_show();
}

static void draw_mppt_screen(void)
{
    char line1[24];
    char line2[24];
    char line3[24];
    char line4[24];

    ssd1306_clear();
    if (history_count == 0)
    {
        ssd1306_draw_string_scaled(TEXT_X_OFFSET, 0, "VE.Direct", 1);
        ssd1306_draw_string_scaled(TEXT_X_OFFSET, 20, "Waiting...", 1);
        ssd1306_show();
        return;
    }

    snprintf(line1, sizeof(line1), "BAT: %.2f V",
             measurement.battery_mv / 1000.0);
    snprintf(line2, sizeof(line2), "PV : %.2f V",
             measurement.panel_mv / 1000.0);
    snprintf(line3, sizeof(line3), "I  : %.2f A",
             measurement.battery_ma / 1000.0);
    snprintf(line4, sizeof(line4), "PV : %ld W", (long)measurement.panel_w);

    ssd1306_draw_string_scaled(TEXT_X_OFFSET, 0, line1, 1);
    ssd1306_draw_string_scaled(TEXT_X_OFFSET, 15, line2, 1);
    ssd1306_draw_string_scaled(TEXT_X_OFFSET, 30, line3, 1);
    ssd1306_draw_string_scaled(TEXT_X_OFFSET, 45, line4, 1);
    ssd1306_show();
}

static void init_hardware_i2c(i2c_inst_t *port, uint sda_pin, uint scl_pin)
{
    i2c_init(port, 400000);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
}

void dashboard_init(void)
{
    init_hardware_i2c(I2C_PORT_OLED_1, SDA_PIN_OLED_1, SCL_PIN_OLED_1);
    init_hardware_i2c(I2C_PORT_OLED_2, SDA_PIN_OLED_2, SCL_PIN_OLED_2);

    ssd1306_init(I2C_PORT_OLED_1);
    ssd1306_show();
    ssd1306_init(I2C_PORT_OLED_2);
    ssd1306_show();
    ssd1306_init_sw(SDA_PIN_OLED_3, SCL_PIN_OLED_3);
    ssd1306_show();
    ssd1306_init_sw(SDA_PIN_OLED_4, SCL_PIN_OLED_4);
    ssd1306_show();
}

void dashboard_accept_measurement(const vedirect_measurement_t *new_measurement)
{
    measurement = *new_measurement;
    add_history_sample(measurement.battery_mv, measurement.panel_mv);
}

void dashboard_render(const vedirect_parser_t *parser)
{
    ssd1306_set_i2c_port(I2C_PORT_OLED_1);
    draw_voltage_graph("BAT", measurement.battery_mv, battery_history,
                       history_count, 500);

    ssd1306_set_i2c_port(I2C_PORT_OLED_2);
    draw_voltage_graph("PV", measurement.panel_mv, panel_history,
                       history_count, 2000);

    ssd1306_set_sw_i2c_pins(SDA_PIN_OLED_3, SCL_PIN_OLED_3);
    draw_counter_screen(parser);

    ssd1306_set_sw_i2c_pins(SDA_PIN_OLED_4, SCL_PIN_OLED_4);
    draw_mppt_screen();
}
