#include <stdio.h>
#include "pico/stdlib.h"

#define LED0 2
#define LED1 3
#define LED2 4
#define LED3 5
#define LED4 6
#define LED5 7
#define LED6 8
#define LED7 9

void print_binary_32(int value) {
    printf("32-bit int: ");
    for (int i = 31; i >= 0; i--) {
        printf("%d", (value >> i) & 1);
        if (i % 8 == 0 && i != 0) printf(" "); 
    }
    printf("\n");
}

int main() {
    stdio_init_all();
    sleep_ms(1000); // wait for USB

    int leds[8] = {LED0, LED1, LED2, LED3, LED4, LED5, LED6, LED7};
    for (int i = 0; i < 8; i++) {
        gpio_init(leds[i]);
        gpio_set_dir(leds[i], GPIO_OUT);
    }

    while (true) {
        int value;
        if (scanf("%d", &value) == 1) {
            printf("Received: %d\n", value);
            print_binary_32(value);

            for (int i = 0; i < 8; i++) {
                gpio_put(leds[i], (value >> i) & 1);
            }
        }
        sleep_ms(100);
    }
}
