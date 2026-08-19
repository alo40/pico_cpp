#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    sleep_ms(2000);  // IMPORTANT

    printf("Pico is alive!\n");

    while (true) {
        printf("Tick...\n");
        sleep_ms(1000);
    }
}
