#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "ssd1306.h"
#include <stdio.h>

#define I2C_PORT i2c0
#define SDA_PIN 0
#define SCL_PIN 1

int main()
{
    stdio_init_all();

    //////////////////////////////////////////////////////
    // Onboard LED
    //////////////////////////////////////////////////////

    const uint LED_PIN = 25;

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    //////////////////////////////////////////////////////
    // I2C setup for OLED
    //////////////////////////////////////////////////////

    i2c_init(I2C_PORT, 400000);

    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    ssd1306_init(I2C_PORT);
    ssd1306_clear();

    //////////////////////////////////////////////////////
    // ADC setup
    //////////////////////////////////////////////////////

    adc_init();

    // Internal temperature sensor
    adc_set_temp_sensor_enabled(true);

    // GPIO29 / ADC3 is connected to VSYS / 3 on the Pico board
    adc_gpio_init(29);

    const float conversion_factor = 3.3f / (1 << 12);

    //////////////////////////////////////////////////////
    // Clock variables
    //////////////////////////////////////////////////////

    int hours = 0;
    int minutes = 0;
    int seconds = 0;

    uint32_t last_time_ms = to_ms_since_boot(get_absolute_time());

    //////////////////////////////////////////////////////
    // FPS variables
    //////////////////////////////////////////////////////

    uint32_t frame_count = 0;
    uint32_t fps = 0;
    uint32_t last_fps_time_ms = to_ms_since_boot(get_absolute_time());

    //////////////////////////////////////////////////////
    // Moving text variables
    //////////////////////////////////////////////////////

    int screen_width = 128;

    int text_x = 0;
    int text_speed = 1;

    const char message[] = "Hello Pico!";

    //////////////////////////////////////////////////////
    // Main loop
    //////////////////////////////////////////////////////

    while (true)
    {
        uint32_t now_ms = to_ms_since_boot(get_absolute_time());

        //////////////////////////////////////////////////////
        // Clock update every 1 second
        //////////////////////////////////////////////////////

        if (now_ms - last_time_ms >= 1000)
        {
            last_time_ms = now_ms;

            seconds++;

            if (seconds >= 60)
            {
                seconds = 0;
                minutes++;

                if (minutes >= 60)
                {
                    minutes = 0;
                    hours++;

                    if (hours >= 24)
                    {
                        hours = 0;
                    }
                }
            }
        }

        //////////////////////////////////////////////////////
        // FPS calculation
        //////////////////////////////////////////////////////

        frame_count++;

        if (now_ms - last_fps_time_ms >= 1000)
        {
            fps = frame_count;
            frame_count = 0;
            last_fps_time_ms = now_ms;
        }

        //////////////////////////////////////////////////////
        // Read internal temperature
        //////////////////////////////////////////////////////

        adc_select_input(4); // ADC4 = internal temperature sensor

        uint16_t raw_temp = adc_read();
        float voltage_temp = raw_temp * conversion_factor;

        float temperature_c = 27.0f - (voltage_temp - 0.706f) / 0.001721f;

        //////////////////////////////////////////////////////
        // Read VSYS voltage
        //////////////////////////////////////////////////////

        adc_select_input(3); // ADC3 = GPIO29 = VSYS / 3

        uint16_t raw_vsys = adc_read();
        float voltage_vsys_adc = raw_vsys * conversion_factor;
        float vsys = voltage_vsys_adc * 3.0f;

        //////////////////////////////////////////////////////
        // Prepare text strings
        //////////////////////////////////////////////////////

        char time_str[16];
        sprintf(time_str, "Time: %02d:%02d:%02d", hours, minutes, seconds);

        char temp_str[20];
        sprintf(temp_str, "Temp: %.1f C", temperature_c);

        char uptime_str[20];
        sprintf(uptime_str, "Up: %lu s", (unsigned long)(now_ms / 1000));

        char fps_str[20];
        sprintf(fps_str, "FPS: %lu", (unsigned long)fps);

        char vsys_str[20];
        sprintf(vsys_str, "VSYS: %.2f V", vsys);

        //////////////////////////////////////////////////////
        // Draw OLED content
        //////////////////////////////////////////////////////

        ssd1306_clear();

        int text_margin = 4;
        ssd1306_draw_string(text_margin, 0, time_str);
        ssd1306_draw_string(text_margin, 8, temp_str);
        ssd1306_draw_string(text_margin, 16, uptime_str);
        ssd1306_draw_string(text_margin, 24, fps_str);
        ssd1306_draw_string(text_margin, 32, vsys_str);

        // Moving bottom message
        ssd1306_draw_string(text_x, 56, message);

        ssd1306_show();

        //////////////////////////////////////////////////////
        // Update moving text position
        //////////////////////////////////////////////////////

        text_x += text_speed;

        // Once the text reaches the right side, restart from the left
        if (text_x > screen_width)
        {
            text_x = 0;
        }

        sleep_ms(10);
    }
}