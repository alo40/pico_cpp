#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"

#define I2C_PORT i2c0
#define SDA_PIN 0
#define SCL_PIN 1

int main() {
  stdio_init_all();

  //////////////////////////////////////////////////////

  // The onboard LED is usually GPIO 25 (Pico)
  const uint LED_PIN = 25;

  // Initialize the LED pin
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  /* while (true) { */
  /*     gpio_put(LED_PIN, 1); // LED ON */
  /*     sleep_ms(500); */

  /*     gpio_put(LED_PIN, 0); // LED OFF */
  /*     sleep_ms(500); */
  /* } */

  //////////////////////////////////////////////////////

  i2c_init(I2C_PORT, 400000);

  gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(SDA_PIN);
  gpio_pull_up(SCL_PIN);

  ssd1306_init(I2C_PORT);
  ssd1306_clear();

  /* // Draw straight line */
  /* for (int i = 0; i < 64; i++) { */
  /*     ssd1306_draw_pixel(i, i, true); */
  /* } */

  void draw_circle(int cx, int cy, int radius) {
    for (int y = -radius; y <= radius; y++) {
      for (int x = -radius; x <= radius; x++) {
	if (x*x + y*y <= radius*radius) {
	  ssd1306_draw_pixel(cx + x, cy + y, true);
	}
      }
    }
  }

  int i = 0;           // position offset
  int vel = 4;         // speed and direction
  int screen_width = 128; // OLED width
  int cx = screen_width / 2;
  int cy = 32;
  int radius = 8;      // circle radius

  while (true) {
    /* gpio_put(LED_PIN, 1); // LED ON */
    /* sleep_ms(100); */

    /* gpio_put(LED_PIN, 0); // LED OFF */
    /* sleep_ms(100); */

    ssd1306_clear(); // Clear previous frame

    // Draw circles
    draw_circle(cx - i, cy, radius); // Left circle
    draw_circle(cx + i, cy, radius); // Right circle

    // 👇 ADD TEXT HERE
    ssd1306_draw_string(0, 0, "HELLO");
    ssd1306_draw_string(0, 10, "123");
    
    ssd1306_show(); // Update OLED

    // Check for edges before moving
    if (cx + i + radius + vel >= screen_width) {
      vel = -vel; // reverse direction
    }

    if (cx + i + radius + vel <= 10) {
      vel = -vel; // reverse direction
    }
    
    i += vel; // move
  }
}
