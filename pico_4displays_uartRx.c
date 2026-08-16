#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include "ssd1306.h"

#include <stdio.h>
#include <stdlib.h>
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
//////////////////////////////////////////////////////

#define SDA_PIN_OLED_3 16
#define SCL_PIN_OLED_3 17

//////////////////////////////////////////////////////
// OLED 4
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
// VE.Direct values
//////////////////////////////////////////////////////

static long battery_mv = 0;
static long panel_mv   = 0;
static long battery_ma = 0;
static long panel_w    = 0;

static bool mppt_data_received = false;

//////////////////////////////////////////////////////
// Text animation
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

//////////////////////////////////////////////////////
// UART interrupt
//////////////////////////////////////////////////////

static void on_uart_rx(void)
{
    while (uart_is_readable(VE_UART))
    {
        uint8_t c = uart_getc(VE_UART);

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
        return false;

    *c = rx_buffer[rx_tail];

    rx_tail =
        (rx_tail + 1) % RX_BUFFER_SIZE;

    return true;
}

//////////////////////////////////////////////////////
// Parse one VE.Direct line
//////////////////////////////////////////////////////

static void process_ve_line(char *line)
{
    char *tab = strchr(line, '\t');

    if (tab == NULL)
        return;

    *tab = '\0';

    char *label = line;
    char *value = tab + 1;

    if (strcmp(label, "V") == 0)
    {
        battery_mv = strtol(value, NULL, 10);
        mppt_data_received = true;
    }
    else if (strcmp(label, "VPV") == 0)
    {
        panel_mv = strtol(value, NULL, 10);
    }
    else if (strcmp(label, "I") == 0)
    {
        battery_ma = strtol(value, NULL, 10);
    }
    else if (strcmp(label, "PPV") == 0)
    {
        panel_w = strtol(value, NULL, 10);
    }
}

//////////////////////////////////////////////////////
// Process received VE.Direct bytes
//////////////////////////////////////////////////////

static void process_ve_direct(void)
{
    static char line[64];
    static size_t position = 0;

    uint8_t c;

    while (ve_get_byte(&c))
    {
        /*
         * VE.Direct lines use:
         *
         * CR LF LABEL TAB VALUE
         */

        if (c == '\r')
            continue;

        if (c == '\n')
        {
            if (position > 0)
            {
                line[position] = '\0';

                process_ve_line(line);

                position = 0;
            }

            continue;
        }

        /*
         * Keep ASCII and TAB characters.
         *
         * This also avoids putting the binary checksum
         * byte into our text parser.
         */
        if ((c >= 32 && c <= 126) || c == '\t')
        {
            if (position < sizeof(line) - 1)
            {
                line[position++] = (char)c;
            }
        }
    }
}

//////////////////////////////////////////////////////
// MPPT display
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
        ssd1306_draw_string_scaled(0, 0, "VE.Direct", 1);
        ssd1306_draw_string_scaled(0, 20, "Waiting...", 1);
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

    ssd1306_draw_string_scaled(0, 0,  line1, 1);
    ssd1306_draw_string_scaled(0, 15, line2, 1);
    ssd1306_draw_string_scaled(0, 30, line3, 1);
    ssd1306_draw_string_scaled(0, 45, line4, 1);

    ssd1306_show();
}

//////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////

int main()
{
    stdio_init_all();

    //////////////////////////////////////////////////////
    // OLED 1
    //////////////////////////////////////////////////////

    i2c_init(I2C_PORT_OLED_1, 400000);

    gpio_set_function(
        SDA_PIN_OLED_1,
        GPIO_FUNC_I2C
    );

    gpio_set_function(
        SCL_PIN_OLED_1,
        GPIO_FUNC_I2C
    );

    gpio_pull_up(SDA_PIN_OLED_1);
    gpio_pull_up(SCL_PIN_OLED_1);

    //////////////////////////////////////////////////////
    // OLED 2
    //////////////////////////////////////////////////////

    i2c_init(I2C_PORT_OLED_2, 400000);

    gpio_set_function(
        SDA_PIN_OLED_2,
        GPIO_FUNC_I2C
    );

    gpio_set_function(
        SCL_PIN_OLED_2,
        GPIO_FUNC_I2C
    );

    gpio_pull_up(SDA_PIN_OLED_2);
    gpio_pull_up(SCL_PIN_OLED_2);

    //////////////////////////////////////////////////////
    // Initialize OLEDs
    //////////////////////////////////////////////////////

    ssd1306_init(I2C_PORT_OLED_1);
    ssd1306_clear();
    ssd1306_show();

    ssd1306_init(I2C_PORT_OLED_2);
    ssd1306_clear();
    ssd1306_show();

    ssd1306_init_sw(
        SDA_PIN_OLED_3,
        SCL_PIN_OLED_3
    );

    ssd1306_clear();
    ssd1306_show();

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

    uart_set_format(
        VE_UART,
        8,
        1,
        UART_PARITY_NONE
    );

    uart_set_hw_flow(
        VE_UART,
        false,
        false
    );

    uart_set_fifo_enabled(
        VE_UART,
        true
    );

    //////////////////////////////////////////////////////
    // Enable UART1 RX interrupt
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
    // Animation
    //////////////////////////////////////////////////////

    const char *screen_text[3] =
    {
        "SCREEN 1",
        "SCREEN 2",
        "SCREEN 3"
    };

    int text_x[3] =
    {
        SSD1306_WIDTH,
        SSD1306_WIDTH,
        SSD1306_WIDTH
    };

    //////////////////////////////////////////////////////
    // Main loop
    //////////////////////////////////////////////////////

    while (true)
    {
        //////////////////////////////////////////////////
        // Process incoming MPPT data
        //////////////////////////////////////////////////

        process_ve_direct();

        //////////////////////////////////////////////////
        // Screen 1
        //////////////////////////////////////////////////

        ssd1306_set_i2c_port(
            I2C_PORT_OLED_1
        );

        draw_screen_text(
            screen_text[0],
            text_x[0]
        );

        //////////////////////////////////////////////////
        // Screen 2
        //////////////////////////////////////////////////

        ssd1306_set_i2c_port(
            I2C_PORT_OLED_2
        );

        draw_screen_text(
            screen_text[1],
            text_x[1]
        );

        //////////////////////////////////////////////////
        // Screen 3
        //////////////////////////////////////////////////

        ssd1306_set_sw_i2c_pins(
            SDA_PIN_OLED_3,
            SCL_PIN_OLED_3
        );

        draw_screen_text(
            screen_text[2],
            text_x[2]
        );

        //////////////////////////////////////////////////
        // Screen 4: MPPT
        //////////////////////////////////////////////////

        ssd1306_set_sw_i2c_pins(
            SDA_PIN_OLED_4,
            SCL_PIN_OLED_4
        );

        draw_mppt_screen();

        //////////////////////////////////////////////////
        // Animation
        //////////////////////////////////////////////////

        for (int i = 0; i < 3; i++)
        {
            text_x[i] -= TEXT_SPEED;

            int width =
                text_width_scaled(
                    screen_text[i],
                    TEXT_SCALE
                );

            if (text_x[i] < -width)
            {
                text_x[i] = SSD1306_WIDTH;
            }
        }

        sleep_ms(20);
    }
}