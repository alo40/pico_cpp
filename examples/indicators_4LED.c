#include <stdio.h>
#include "pico/stdlib.h"

#define LED0 2
#define LED1 3
#define LED2 4
#define LED3 5


// Function to print 32-bit binary
void print_binary_32(int value) {
    printf("32-bit int: ");
    for (int i = 31; i >= 0; i--) {
        printf("%d", (value >> i) & 1); // extract each bit
        if (i % 8 == 0 && i != 0) printf(" "); // separate each byte
    }
    printf("\n");
}


int main() {
    stdio_init_all();

    // Initialize LED pins
    gpio_init(LED0);
    gpio_set_dir(LED0, GPIO_OUT);

    gpio_init(LED1);
    gpio_set_dir(LED1, GPIO_OUT);

    gpio_init(LED2);
    gpio_set_dir(LED2, GPIO_OUT);

    gpio_init(LED3);
    gpio_set_dir(LED3, GPIO_OUT);

    while (true) {
        int value; // 32 bits

        // Wait for input from USB
        if (scanf("%d", &value) == 1) {
            printf("Recibido (decimal): %d\n", value);
            print_binary_32(value);

            // Set LEDs based on binary value
            gpio_put(LED0, (value >> 0) & 1);
            gpio_put(LED1, (value >> 1) & 1);
            gpio_put(LED2, (value >> 2) & 1);
            gpio_put(LED3, (value >> 3) & 1);
        }

        sleep_ms(100);
    }
}
