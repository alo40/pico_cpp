#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include "ssd1306.h"
#include "vedirect_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

//////////////////////////////////////////////////////
// GENERAL
//////////////////////////////////////////////////////

#define TEXT_X_OFFSET 2

//////////////////////////////////////////////////////
// OLED 1
// Battery voltage graph
//////////////////////////////////////////////////////

#define I2C_PORT_OLED_1 i2c0
#define SDA_PIN_OLED_1 0
#define SCL_PIN_OLED_1 1

//////////////////////////////////////////////////////
// OLED 2
// PV voltage graph
//////////////////////////////////////////////////////

#define I2C_PORT_OLED_2 i2c1
#define SDA_PIN_OLED_2 2
#define SCL_PIN_OLED_2 3

//////////////////////////////////////////////////////
// OLED 3
// VE.Direct counters
//////////////////////////////////////////////////////

#define SDA_PIN_OLED_3 16
#define SCL_PIN_OLED_3 17

//////////////////////////////////////////////////////
// OLED 4
// MPPT current values
//////////////////////////////////////////////////////

#define SDA_PIN_OLED_4 18
#define SCL_PIN_OLED_4 19

//////////////////////////////////////////////////////
// VE.Direct
//////////////////////////////////////////////////////

#define VE_UART        uart1
#define VE_BAUD        19200
#define VE_RX_PIN      5

#define RX_BUFFER_SIZE 512

static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

//////////////////////////////////////////////////////
// VE.Direct counters
//////////////////////////////////////////////////////

static volatile uint32_t uart_irq_count = 0;
static volatile uint32_t uart_rx_byte_count = 0;

static vedirect_parser_t ve_parser;

//////////////////////////////////////////////////////
// VE.Direct values
//////////////////////////////////////////////////////

static long battery_mv = 0;
static long panel_mv   = 0;
static long battery_ma = 0;
static long panel_w    = 0;

static bool mppt_data_received = false;

//////////////////////////////////////////////////////
// Voltage history
//////////////////////////////////////////////////////

/*
 * One history sample uses one horizontal OLED pixel.
 *
 * 128 samples × ~1 sample/second
 * gives approximately 128 seconds of history.
 */
#define HISTORY_SIZE SSD1306_WIDTH

static long battery_history[HISTORY_SIZE];
static long panel_history[HISTORY_SIZE];

static size_t history_count = 0;

//////////////////////////////////////////////////////
// Graph area
//////////////////////////////////////////////////////

/*
 * Top 9 pixels are reserved for the title/value.
 *
 * Graph occupies:
 *
 * Y = 10 ... 63
 */
#define GRAPH_TOP    10
#define GRAPH_BOTTOM (SSD1306_HEIGHT - 1)

//////////////////////////////////////////////////////
// Add a new voltage history sample
//////////////////////////////////////////////////////

static void add_history_sample(
    long battery,
    long panel
)
{
    /*
     * While the history is not full,
     * simply append samples.
     */
    if (history_count < HISTORY_SIZE)
    {
        battery_history[history_count] = battery;
        panel_history[history_count]   = panel;

        history_count++;

        return;
    }

    /*
     * History is full.
     *
     * Move everything one position left:
     *
     * old:
     *
     * [0][1][2][3]...[127]
     *
     * becomes:
     *
     * [1][2][3]...[127][NEW]
     */

    memmove(
        &battery_history[0],
        &battery_history[1],
        (HISTORY_SIZE - 1) * sizeof(battery_history[0])
    );

    memmove(
        &panel_history[0],
        &panel_history[1],
        (HISTORY_SIZE - 1) * sizeof(panel_history[0])
    );

    battery_history[HISTORY_SIZE - 1] = battery;
    panel_history[HISTORY_SIZE - 1]   = panel;
}

//////////////////////////////////////////////////////
// Draw a line
//////////////////////////////////////////////////////

/*
 * Simple Bresenham line algorithm.
 *
 * Our SSD1306 driver already works with pixels, so
 * this allows us to connect the voltage samples
 * into a continuous graph.
 */
static void draw_line(
    int x0,
    int y0,
    int x1,
    int y1
)
{
    int dx = abs(x1 - x0);
    int sx = (x0 < x1) ? 1 : -1;

    int dy = -abs(y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;

    int error = dx + dy;

    while (true)
    {
        ssd1306_draw_pixel(
            x0,
            y0,
            true
        );

        if (
            x0 == x1 &&
            y0 == y1
        )
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

//////////////////////////////////////////////////////
// Calculate graph scale
//////////////////////////////////////////////////////

static void calculate_graph_range(
    const long *history,
    size_t count,
    long minimum_span,
    long *minimum,
    long *maximum
)
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

    long span =
        max_value - min_value;

    //////////////////////////////////////////////////
    // Avoid excessively magnifying tiny changes
    //////////////////////////////////////////////////

    if (span < minimum_span)
    {
        long center =
            (min_value + max_value) / 2;

        min_value =
            center - minimum_span / 2;

        max_value =
            center + minimum_span / 2;
    }
    else
    {
        /*
         * Add approximately 10% vertical margin.
         */
        long margin =
            span / 10;

        min_value -= margin;
        max_value += margin;
    }

    //////////////////////////////////////////////////
    // Voltage should not go below 0
    //////////////////////////////////////////////////

    if (min_value < 0)
    {
        min_value = 0;
    }

    //////////////////////////////////////////////////
    // Prevent divide-by-zero
    //////////////////////////////////////////////////

    if (max_value <= min_value)
    {
        max_value =
            min_value + 1;
    }

    *minimum = min_value;
    *maximum = max_value;
}

//////////////////////////////////////////////////////
// Convert voltage to graph Y coordinate
//////////////////////////////////////////////////////

static int voltage_to_y(
    long value,
    long minimum,
    long maximum
)
{
    //////////////////////////////////////////////////
    // Clamp value
    //////////////////////////////////////////////////

    if (value < minimum)
    {
        value = minimum;
    }

    if (value > maximum)
    {
        value = maximum;
    }

    long range =
        maximum - minimum;

    int graph_height =
        GRAPH_BOTTOM - GRAPH_TOP;

    /*
     * Use int64_t for the intermediate
     * multiplication.
     */
    int y =
        GRAPH_BOTTOM -
        (int)(
            ((int64_t)(value - minimum) * graph_height)
            / range
        );

    return y;
}

//////////////////////////////////////////////////////
// Draw voltage graph
//////////////////////////////////////////////////////

static void draw_voltage_graph(
    const char *name,
    long current_mv,
    const long *history,
    size_t count,
    long minimum_span_mv
)
{
    char title[24];

    ssd1306_clear();

    //////////////////////////////////////////////////
    // Waiting for data
    //////////////////////////////////////////////////

    if (count == 0)
    {
        ssd1306_draw_string_scaled(
            TEXT_X_OFFSET,
            0,
            name,
            1
        );

        ssd1306_draw_string_scaled(
            TEXT_X_OFFSET,
            20,
            "Waiting...",
            1
        );

        ssd1306_show();

        return;
    }

    //////////////////////////////////////////////////
    // Display current voltage
    //////////////////////////////////////////////////

    snprintf(
        title,
        sizeof(title),
        "%s %.2fV",
        name,
        current_mv / 1000.0
    );

    ssd1306_draw_string_scaled(
        TEXT_X_OFFSET,
        0,
        title,
        1
    );

    //////////////////////////////////////////////////
    // Determine automatic Y scale
    //////////////////////////////////////////////////

    long graph_min;
    long graph_max;

    calculate_graph_range(
        history,
        count,
        minimum_span_mv,
        &graph_min,
        &graph_max
    );

    //////////////////////////////////////////////////
    // Draw top/bottom dotted guides
    //////////////////////////////////////////////////

    for (int x = 0; x < SSD1306_WIDTH; x += 8)
    {
        ssd1306_draw_pixel(
            x,
            GRAPH_TOP,
            true
        );

        ssd1306_draw_pixel(
            x,
            GRAPH_BOTTOM,
            true
        );
    }

    //////////////////////////////////////////////////
    // Draw history
    //////////////////////////////////////////////////

    if (count == 1)
    {
        int y =
            voltage_to_y(
                history[0],
                graph_min,
                graph_max
            );

        ssd1306_draw_pixel(
            0,
            y,
            true
        );
    }
    else
    {
        for (size_t i = 1; i < count; i++)
        {
            int x0 =
                (int)i - 1;

            int x1 =
                (int)i;

            int y0 =
                voltage_to_y(
                    history[i - 1],
                    graph_min,
                    graph_max
                );

            int y1 =
                voltage_to_y(
                    history[i],
                    graph_min,
                    graph_max
                );

            draw_line(
                x0,
                y0,
                x1,
                y1
            );
        }
    }

    ssd1306_show();
}

//////////////////////////////////////////////////////
// Battery voltage graph
//////////////////////////////////////////////////////

static void draw_battery_graph(void)
{
    /*
     * Minimum displayed range = 0.5 V.
     *
     * This avoids turning tiny millivolt noise
     * into huge vertical movements.
     */
    draw_voltage_graph(
        "BAT",
        battery_mv,
        battery_history,
        history_count,
        500
    );
}

//////////////////////////////////////////////////////
// PV voltage graph
//////////////////////////////////////////////////////

static void draw_panel_graph(void)
{
    /*
     * Minimum displayed range = 2 V.
     *
     * PV voltage normally varies more than
     * battery voltage.
     */
    draw_voltage_graph(
        "PV",
        panel_mv,
        panel_history,
        history_count,
        2000
    );
}

//////////////////////////////////////////////////////
// UART interrupt
//////////////////////////////////////////////////////

static void on_uart_rx(void)
{
    /*
     * Count interrupt-handler executions.
     */
    uart_irq_count++;

    while (uart_is_readable(VE_UART))
    {
        uint8_t c =
            uart_getc(VE_UART);

        /*
         * Count every UART byte.
         */
        uart_rx_byte_count++;

        uint16_t next =
            (rx_head + 1) % RX_BUFFER_SIZE;

        if (next != rx_tail)
        {
            rx_buffer[rx_head] = c;
            rx_head = next;
        }
    }
}

//////////////////////////////////////////////////////
// Get byte from receive buffer
//////////////////////////////////////////////////////

static bool ve_get_byte(uint8_t *c)
{
    if (rx_head == rx_tail)
    {
        return false;
    }

    *c =
        rx_buffer[rx_tail];

    rx_tail =
        (rx_tail + 1) % RX_BUFFER_SIZE;

    return true;
}

//////////////////////////////////////////////////////
// Process received VE.Direct bytes
//////////////////////////////////////////////////////

static void process_ve_direct(void)
{
    uint8_t c;

    while (ve_get_byte(&c))
    {
        vedirect_measurement_t measurement;

        if (
            vedirect_parser_feed(
                &ve_parser,
                c,
                &measurement
            ) == VEDIRECT_VALID_BLOCK
        )
        {
            battery_mv = measurement.battery_mv;
            panel_mv = measurement.panel_mv;
            battery_ma = measurement.battery_ma;
            panel_w = measurement.panel_w;
            mppt_data_received = true;

            add_history_sample(
                battery_mv,
                panel_mv
            );
        }
    }
}

//////////////////////////////////////////////////////
// OLED 3
// VE.Direct counters
//////////////////////////////////////////////////////

static void draw_counter_screen(void)
{
    char line1[24];
    char line2[24];
    char line3[24];

    uint32_t received_blocks =
        ve_parser.received_blocks;

    uint32_t valid_blocks =
        ve_parser.valid_blocks;

    uint32_t invalid_checksum_blocks =
        ve_parser.invalid_checksum_blocks;

    uint32_t incomplete_blocks =
        ve_parser.incomplete_blocks;

    snprintf(
        line1,
        sizeof(line1),
        "RX: %lu",
        (unsigned long)received_blocks
    );

    snprintf(
        line2,
        sizeof(line2),
        "OK: %lu",
        (unsigned long)valid_blocks
    );

    snprintf(
        line3,
        sizeof(line3),
        "CK:%lu IN:%lu",
        (unsigned long)invalid_checksum_blocks,
        (unsigned long)incomplete_blocks
    );

    ssd1306_clear();

    ssd1306_draw_string_scaled(
        TEXT_X_OFFSET,
        0,
        "VE.Direct",
        1
    );

    ssd1306_draw_string_scaled(
        TEXT_X_OFFSET,
        15,
        line1,
        1
    );

    ssd1306_draw_string_scaled(
        TEXT_X_OFFSET,
        30,
        line2,
        1
    );

    ssd1306_draw_string_scaled(
        TEXT_X_OFFSET,
        45,
        line3,
        1
    );

    ssd1306_show();
}

//////////////////////////////////////////////////////
// OLED 4
// MPPT values
//////////////////////////////////////////////////////

static void draw_mppt_screen(void)
{
    char line1[24];
    char line2[24];
    char line3[24];
    char line4[24];

    if (!mppt_data_received)
    {
        ssd1306_clear();

        ssd1306_draw_string_scaled(
            TEXT_X_OFFSET,
            0,
            "VE.Direct",
            1
        );

        ssd1306_draw_string_scaled(
            TEXT_X_OFFSET,
            20,
            "Waiting...",
            1
        );

        ssd1306_show();

        return;
    }

    snprintf(
        line1,
        sizeof(line1),
        "BAT: %.2f V",
        battery_mv / 1000.0
    );

    snprintf(
        line2,
        sizeof(line2),
        "PV : %.2f V",
        panel_mv / 1000.0
    );

    snprintf(
        line3,
        sizeof(line3),
        "I  : %.2f A",
        battery_ma / 1000.0
    );

    snprintf(
        line4,
        sizeof(line4),
        "PV : %ld W",
        panel_w
    );

    ssd1306_clear();

    ssd1306_draw_string_scaled(
        TEXT_X_OFFSET,
        0,
        line1,
        1
    );

    ssd1306_draw_string_scaled(
        TEXT_X_OFFSET,
        15,
        line2,
        1
    );

    ssd1306_draw_string_scaled(
        TEXT_X_OFFSET,
        30,
        line3,
        1
    );

    ssd1306_draw_string_scaled(
        TEXT_X_OFFSET,
        45,
        line4,
        1
    );

    ssd1306_show();
}

//////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////

int main()
{
    stdio_init_all();
    vedirect_parser_init(&ve_parser);

    //////////////////////////////////////////////////////
    // OLED 1
    //////////////////////////////////////////////////////

    i2c_init(
        I2C_PORT_OLED_1,
        400000
    );

    gpio_set_function(
        SDA_PIN_OLED_1,
        GPIO_FUNC_I2C
    );

    gpio_set_function(
        SCL_PIN_OLED_1,
        GPIO_FUNC_I2C
    );

    gpio_pull_up(
        SDA_PIN_OLED_1
    );

    gpio_pull_up(
        SCL_PIN_OLED_1
    );

    //////////////////////////////////////////////////////
    // OLED 2
    //////////////////////////////////////////////////////

    i2c_init(
        I2C_PORT_OLED_2,
        400000
    );

    gpio_set_function(
        SDA_PIN_OLED_2,
        GPIO_FUNC_I2C
    );

    gpio_set_function(
        SCL_PIN_OLED_2,
        GPIO_FUNC_I2C
    );

    gpio_pull_up(
        SDA_PIN_OLED_2
    );

    gpio_pull_up(
        SCL_PIN_OLED_2
    );

    //////////////////////////////////////////////////////
    // Initialize OLED 1
    //////////////////////////////////////////////////////

    ssd1306_init(
        I2C_PORT_OLED_1
    );

    ssd1306_clear();
    ssd1306_show();

    //////////////////////////////////////////////////////
    // Initialize OLED 2
    //////////////////////////////////////////////////////

    ssd1306_init(
        I2C_PORT_OLED_2
    );

    ssd1306_clear();
    ssd1306_show();

    //////////////////////////////////////////////////////
    // Initialize OLED 3
    // Software I2C
    //////////////////////////////////////////////////////

    ssd1306_init_sw(
        SDA_PIN_OLED_3,
        SCL_PIN_OLED_3
    );

    ssd1306_clear();
    ssd1306_show();

    //////////////////////////////////////////////////////
    // Initialize OLED 4
    // Software I2C
    //////////////////////////////////////////////////////

    ssd1306_init_sw(
        SDA_PIN_OLED_4,
        SCL_PIN_OLED_4
    );

    ssd1306_clear();
    ssd1306_show();

    //////////////////////////////////////////////////////
    // Initialize VE.Direct UART
    //////////////////////////////////////////////////////

    uart_init(
        VE_UART,
        VE_BAUD
    );

    gpio_set_function(
        VE_RX_PIN,
        GPIO_FUNC_UART
    );

    //////////////////////////////////////////////////////
    // 19200 baud
    // 8 data bits
    // no parity
    // 1 stop bit
    //////////////////////////////////////////////////////

    uart_set_format(
        VE_UART,
        8,
        1,
        UART_PARITY_NONE
    );

    //////////////////////////////////////////////////////
    // No hardware flow control
    //////////////////////////////////////////////////////

    uart_set_hw_flow(
        VE_UART,
        false,
        false
    );

    //////////////////////////////////////////////////////
    // Enable UART FIFO
    //////////////////////////////////////////////////////

    uart_set_fifo_enabled(
        VE_UART,
        true
    );

    //////////////////////////////////////////////////////
    // Enable UART RX interrupt
    //////////////////////////////////////////////////////

    irq_set_exclusive_handler(
        UART1_IRQ,
        on_uart_rx
    );

    irq_set_enabled(
        UART1_IRQ,
        true
    );

    uart_set_irq_enables(
        VE_UART,
        true,
        false
    );

    //////////////////////////////////////////////////////
    // Main loop
    //////////////////////////////////////////////////////

    while (true)
    {
        //////////////////////////////////////////////////
        // Process incoming VE.Direct bytes
        //////////////////////////////////////////////////

        process_ve_direct();

        //////////////////////////////////////////////////
        // OLED 1
        // Battery voltage history
        //////////////////////////////////////////////////

        ssd1306_set_i2c_port(
            I2C_PORT_OLED_1
        );

        draw_battery_graph();

        //////////////////////////////////////////////////
        // OLED 2
        // PV voltage history
        //////////////////////////////////////////////////

        ssd1306_set_i2c_port(
            I2C_PORT_OLED_2
        );

        draw_panel_graph();

        //////////////////////////////////////////////////
        // OLED 3
        // Interrupt / RX / block counters
        //////////////////////////////////////////////////

        ssd1306_set_sw_i2c_pins(
            SDA_PIN_OLED_3,
            SCL_PIN_OLED_3
        );

        draw_counter_screen();

        //////////////////////////////////////////////////
        // OLED 4
        // Current MPPT measurements
        //////////////////////////////////////////////////

        ssd1306_set_sw_i2c_pins(
            SDA_PIN_OLED_4,
            SCL_PIN_OLED_4
        );

        draw_mppt_screen();

        //////////////////////////////////////////////////
        // Refresh delay
        //////////////////////////////////////////////////

        sleep_ms(20);
    }
}
