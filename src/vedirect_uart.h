#ifndef VEDIRECT_UART_H
#define VEDIRECT_UART_H

#include "vedirect_parser.h"

#include <stdbool.h>

void vedirect_uart_init(void);
bool vedirect_uart_poll(vedirect_measurement_t *measurement);
const vedirect_parser_t *vedirect_uart_parser(void);

#endif
