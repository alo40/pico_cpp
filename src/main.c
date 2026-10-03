#include "dashboard.h"
#include "vedirect_uart.h"

#include "pico/stdlib.h"

#include <stdio.h>

int main(void)
{
    uint32_t sequence = 0;
    vedirect_measurement_t measurement;

    stdio_init_all();
    printf("sequence,battery_mv,panel_mv,battery_ma,panel_w\n");

    dashboard_init();
    vedirect_uart_init();

    while (true)
    {
        while (vedirect_uart_poll(&measurement))
        {
            dashboard_accept_measurement(&measurement);
            printf("%lu,%ld,%ld,%ld,%ld\n", (unsigned long)++sequence,
                   (long)measurement.battery_mv, (long)measurement.panel_mv,
                   (long)measurement.battery_ma, (long)measurement.panel_w);
        }

        dashboard_render(vedirect_uart_parser());
        sleep_ms(20);
    }
}
