#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    stdio_init_all();

    while (true) {
        int value;

        // Wait for input from USB
        if (scanf("%d", &value) == 1) {
            printf("Recibido: %d\n", value);
        }

        sleep_ms(100);
    }
}
