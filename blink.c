#include "pico/stdlib.h"

int main() {
    // Initialize all standard I/O (optional but common)
    stdio_init_all();

    // The onboard LED is usually GPIO 25 (Pico)
    const uint LED_PIN = 25;

    // Initialize the LED pin
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        gpio_put(LED_PIN, 1); // LED ON
        sleep_ms(500);

        gpio_put(LED_PIN, 0); // LED OFF
        sleep_ms(500);
    }
}
