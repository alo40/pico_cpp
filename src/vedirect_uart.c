#include "vedirect_uart.h"

#include "hardware/irq.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

#include <stdint.h>

#define VE_UART uart1
#define VE_BAUD 19200
#define VE_RX_PIN 5
#define RX_BUFFER_SIZE 512

static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;
static vedirect_parser_t parser;

static void on_uart_rx(void)
{
    while (uart_is_readable(VE_UART))
    {
        uint8_t byte = uart_getc(VE_UART);
        uint16_t next = (rx_head + 1) % RX_BUFFER_SIZE;

        if (next != rx_tail)
        {
            rx_buffer[rx_head] = byte;
            rx_head = next;
        }
    }
}

static bool get_byte(uint8_t *byte)
{
    if (rx_head == rx_tail)
    {
        return false;
    }

    *byte = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
    return true;
}

void vedirect_uart_init(void)
{
    vedirect_parser_init(&parser);

    uart_init(VE_UART, VE_BAUD);
    gpio_set_function(VE_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(VE_UART, 8, 1, UART_PARITY_NONE);
    uart_set_hw_flow(VE_UART, false, false);
    uart_set_fifo_enabled(VE_UART, true);

    irq_set_exclusive_handler(UART1_IRQ, on_uart_rx);
    irq_set_enabled(UART1_IRQ, true);
    uart_set_irq_enables(VE_UART, true, false);
}

bool vedirect_uart_poll(vedirect_measurement_t *measurement)
{
    uint8_t byte;

    while (get_byte(&byte))
    {
        if (vedirect_parser_feed(&parser, byte, measurement) ==
            VEDIRECT_VALID_BLOCK)
        {
            return true;
        }
    }

    return false;
}

const vedirect_parser_t *vedirect_uart_parser(void)
{
    return &parser;
}
