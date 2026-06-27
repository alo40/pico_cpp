#include "hardware/i2c.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "ssd1306.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//////////////////////////////////////////////////////
// OLED 1: State machine screen
//////////////////////////////////////////////////////

#define I2C_PORT_OLED_1 i2c0
#define SDA_PIN_OLED_1 0
#define SCL_PIN_OLED_1 1

//////////////////////////////////////////////////////
// OLED 2: Safety monitor screen
//////////////////////////////////////////////////////

#define I2C_PORT_OLED_2 i2c1
#define SDA_PIN_OLED_2 2
#define SCL_PIN_OLED_2 3

//////////////////////////////////////////////////////
// General configuration
//////////////////////////////////////////////////////

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define FRAME_PERIOD_MS        100
#define WATCHDOG_TIMEOUT_MS   2000
#define OLED_REINIT_PERIOD_MS 60000

#define INIT_HOLD_TICKS   8
#define CHECK_HOLD_TICKS  8
#define RUN_HOLD_TICKS   16
#define SAFE_HOLD_TICKS  10

#define ANIM_STEPS 6
#define FAULT_INJECTION_PERIOD 60   // demo fault every N loops

//////////////////////////////////////////////////////
// State machine definition
//////////////////////////////////////////////////////

typedef enum
{
    SM_INIT = 0,
    SM_CHECK,
    SM_RUN,
    SM_SAFE,
    SM_COUNT
} sm_state_t;

typedef struct
{
    int x;
    int y;
    int w;
    int h;
    const char *label;
} state_box_t;

typedef struct
{
    sm_state_t current;
    sm_state_t previous;

    sm_state_t anim_from;
    sm_state_t anim_to;

    bool anim_active;
    uint8_t anim_step;

    uint32_t loops;
    uint32_t transitions;
    uint32_t faults;
    uint32_t state_ticks;

    bool fault_pending;
    bool watchdog_reboot;

    uint32_t last_reinit_ms;
} system_status_t;

static const state_box_t g_state_boxes[SM_COUNT] =
{
    [SM_INIT]  = {  4, 14, 28, 12, "INIT"  },
    [SM_CHECK] = { 44, 14, 36, 12, "CHECK" },
    [SM_RUN]   = { 92, 14, 28, 12, "RUN"   },
    [SM_SAFE]  = { 48, 40, 32, 12, "SAFE"  }
};

//////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////

static int iabs_int(int v)
{
    return (v < 0) ? -v : v;
}

static int imin_int(int a, int b)
{
    return (a < b) ? a : b;
}

static int imax_int(int a, int b)
{
    return (a > b) ? a : b;
}

static const char *state_name(sm_state_t state)
{
    switch (state)
    {
        case SM_INIT:  return "INIT";
        case SM_CHECK: return "CHECK";
        case SM_RUN:   return "RUN";
        case SM_SAFE:  return "SAFE";
        default:       return "UNKNOWN";
    }
}

static bool state_is_valid(sm_state_t state)
{
    return (state >= SM_INIT) && (state < SM_COUNT);
}

static bool system_status_is_valid(const system_status_t *status)
{
    if (!state_is_valid(status->current))   return false;
    if (!state_is_valid(status->previous))  return false;
    if (!state_is_valid(status->anim_from)) return false;
    if (!state_is_valid(status->anim_to))   return false;

    if (status->anim_step > ANIM_STEPS) return false;

    return true;
}

//////////////////////////////////////////////////////
// Drawing primitives
//////////////////////////////////////////////////////

static void oled_draw_line(int x0, int y0, int x1, int y1, bool color)
{
    int dx = iabs_int(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;

    int dy = -iabs_int(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;

    int err = dx + dy;

    while (true)
    {
        ssd1306_draw_pixel(x0, y0, color);

        if (x0 == x1 && y0 == y1)
        {
            break;
        }

        int e2 = 2 * err;

        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

static void oled_draw_rect(int x, int y, int w, int h, bool color)
{
    oled_draw_line(x, y, x + w - 1, y, color);
    oled_draw_line(x, y, x, y + h - 1, color);
    oled_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
    oled_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
}

static void oled_fill_rect(int x, int y, int w, int h, bool color)
{
    for (int dx = 0; dx < w; dx++)
    {
        for (int dy = 0; dy < h; dy++)
        {
            ssd1306_draw_pixel(x + dx, y + dy, color);
        }
    }
}

static void oled_fill_circle(int cx, int cy, int r, bool color)
{
    for (int y = -r; y <= r; y++)
    {
        for (int x = -r; x <= r; x++)
        {
            if ((x * x) + (y * y) <= r * r)
            {
                ssd1306_draw_pixel(cx + x, cy + y, color);
            }
        }
    }
}

//////////////////////////////////////////////////////
// I2C / OLED initialization
//////////////////////////////////////////////////////

static void mission_i2c_init(i2c_inst_t *i2c_port, uint sda_pin, uint scl_pin)
{
    i2c_init(i2c_port, 400000);

    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);

    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
}

static void mission_oled_init_all(void)
{
    ssd1306_set_i2c_port(I2C_PORT_OLED_1);
    ssd1306_init(I2C_PORT_OLED_1);
    ssd1306_clear();
    ssd1306_show();

    watchdog_update();

    ssd1306_set_i2c_port(I2C_PORT_OLED_2);
    ssd1306_init(I2C_PORT_OLED_2);
    ssd1306_clear();
    ssd1306_show();

    watchdog_update();
}

//////////////////////////////////////////////////////
// State machine logic
//////////////////////////////////////////////////////

static void system_status_reset(system_status_t *status, bool watchdog_reboot)
{
    status->current = SM_INIT;
    status->previous = SM_INIT;

    status->anim_from = SM_INIT;
    status->anim_to = SM_INIT;

    status->anim_active = false;
    status->anim_step = 0;

    status->loops = 0;
    status->transitions = 0;
    status->faults = 0;
    status->state_ticks = 0;

    status->fault_pending = false;
    status->watchdog_reboot = watchdog_reboot;

    status->last_reinit_ms = to_ms_since_boot(get_absolute_time());
}

static void enter_state(system_status_t *status, sm_state_t next_state)
{
    status->previous = status->current;
    status->current = next_state;

    status->anim_from = status->previous;
    status->anim_to = next_state;
    status->anim_active = (status->previous != next_state);
    status->anim_step = 0;

    status->state_ticks = 0;
    status->transitions++;

    if (next_state == SM_SAFE)
    {
        status->faults++;
    }
}

static void update_state_machine(system_status_t *status)
{
    status->loops++;
    status->state_ticks++;

    if ((status->loops % FAULT_INJECTION_PERIOD) == 0)
    {
        status->fault_pending = true;
    }

    if (status->anim_active)
    {
        if (status->anim_step < ANIM_STEPS)
        {
            status->anim_step++;
        }
        else
        {
            status->anim_active = false;
        }
    }

    switch (status->current)
    {
        case SM_INIT:
            if (status->state_ticks >= INIT_HOLD_TICKS)
            {
                enter_state(status, SM_CHECK);
            }
            break;

        case SM_CHECK:
            if (status->state_ticks >= CHECK_HOLD_TICKS)
            {
                if (status->fault_pending)
                {
                    status->fault_pending = false;
                    enter_state(status, SM_SAFE);
                }
                else
                {
                    enter_state(status, SM_RUN);
                }
            }
            break;

        case SM_RUN:
            if (status->state_ticks >= RUN_HOLD_TICKS)
            {
                enter_state(status, SM_CHECK);
            }
            break;

        case SM_SAFE:
            if (status->state_ticks >= SAFE_HOLD_TICKS)
            {
                enter_state(status, SM_CHECK);
            }
            break;

        default:
            enter_state(status, SM_INIT);
            break;
    }
}

//////////////////////////////////////////////////////
// Diagram drawing
//////////////////////////////////////////////////////

static int state_center_x(sm_state_t state)
{
    return g_state_boxes[state].x + (g_state_boxes[state].w / 2);
}

static int state_center_y(sm_state_t state)
{
    return g_state_boxes[state].y + (g_state_boxes[state].h / 2);
}

static void draw_state_box(sm_state_t state, bool active)
{
    const state_box_t *b = &g_state_boxes[state];

    oled_draw_rect(b->x, b->y, b->w, b->h, true);

    if (active)
    {
        oled_draw_rect(b->x - 1, b->y - 1, b->w + 2, b->h + 2, true);
        ssd1306_draw_string(b->x - 7, b->y + 2, ">");
    }

    int text_x = b->x + (b->w - ((int)strlen(b->label) * 6)) / 2;
    int text_y = b->y + 2;

    ssd1306_draw_string(text_x, text_y, b->label);
}

static void draw_state_connections(void)
{
    // INIT -> CHECK
    oled_draw_line(32, 20, 44, 20, true);

    // CHECK -> RUN
    oled_draw_line(80, 20, 92, 20, true);

    // RUN -> SAFE
    oled_draw_line(106, 26, 80, 40, true);

    // SAFE -> CHECK
    oled_draw_line(64, 40, 62, 26, true);
}

static void draw_transition_animation(const system_status_t *status)
{
    if (!status->anim_active)
    {
        return;
    }

    int x0 = state_center_x(status->anim_from);
    int y0 = state_center_y(status->anim_from);

    int x1 = state_center_x(status->anim_to);
    int y1 = state_center_y(status->anim_to);

    int step = imin_int(status->anim_step, ANIM_STEPS);

    int dot_x = x0 + ((x1 - x0) * step) / ANIM_STEPS;
    int dot_y = y0 + ((y1 - y0) * step) / ANIM_STEPS;

    oled_fill_circle(dot_x, dot_y, 2, true);
}

static void draw_state_machine_screen(const system_status_t *status)
{
    char line[22];

    ssd1306_clear();

    ssd1306_draw_string(14, 0, "STATE MACHINE");

    draw_state_connections();

    draw_state_box(SM_INIT,  status->current == SM_INIT);
    draw_state_box(SM_CHECK, status->current == SM_CHECK);
    draw_state_box(SM_RUN,   status->current == SM_RUN);
    draw_state_box(SM_SAFE,  status->current == SM_SAFE);

    draw_transition_animation(status);

    snprintf(line, sizeof(line), "CUR: %s", state_name(status->current));
    ssd1306_draw_string(20, 56, line);

    ssd1306_show();
}

//////////////////////////////////////////////////////
// Safety screen drawing
//////////////////////////////////////////////////////

static void draw_safety_screen(const system_status_t *status)
{
    char line[24];
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    uint32_t reinit_age_s = (now_ms - status->last_reinit_ms) / 1000;

    const int x_shift = 2;

    ssd1306_clear();

    ssd1306_draw_string(14, 0, "SAFETY MONITOR");

    snprintf(line, sizeof(line), "CUR   : %s", state_name(status->current));
    ssd1306_draw_string(0 + x_shift, 8, line);

    snprintf(line, sizeof(line), "PREV  : %s", state_name(status->previous));
    ssd1306_draw_string(0 + x_shift, 16, line);

    snprintf(line, sizeof(line), "VALID : %s",
             system_status_is_valid(status) ? "YES" : "NO");
    ssd1306_draw_string(0 + x_shift, 24, line);

    snprintf(line, sizeof(line), "WDOG  : %s",
             status->watchdog_reboot ? "REBOOT" : "COLD");
    ssd1306_draw_string(0 + x_shift, 32, line);

    snprintf(line, sizeof(line), "TRANS : %lu", (unsigned long)status->transitions);
    ssd1306_draw_string(0 + x_shift, 40, line);

    snprintf(line, sizeof(line), "FAULTS: %lu", (unsigned long)status->faults);
    ssd1306_draw_string(0 + x_shift, 48, line);

    snprintf(line, sizeof(line), "REINIT: %lus", (unsigned long)reinit_age_s);
    ssd1306_draw_string(0 + x_shift, 56, line);

    ssd1306_show();
}

//////////////////////////////////////////////////////
// Safe delay
//////////////////////////////////////////////////////

static void mission_delay_ms(uint32_t delay_ms)
{
    uint32_t elapsed = 0;

    while (elapsed < delay_ms)
    {
        watchdog_update();
        sleep_ms(10);
        elapsed += 10;
    }
}

//////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////

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
    // Watchdog
    //////////////////////////////////////////////////////

    bool wd_reboot = watchdog_caused_reboot();
    watchdog_enable(WATCHDOG_TIMEOUT_MS, true);
    watchdog_update();

    //////////////////////////////////////////////////////
    // I2C init
    //////////////////////////////////////////////////////

    mission_i2c_init(I2C_PORT_OLED_1, SDA_PIN_OLED_1, SCL_PIN_OLED_1);
    mission_i2c_init(I2C_PORT_OLED_2, SDA_PIN_OLED_2, SCL_PIN_OLED_2);

    //////////////////////////////////////////////////////
    // OLED init
    //////////////////////////////////////////////////////

    mission_oled_init_all();

    //////////////////////////////////////////////////////
    // Status init
    //////////////////////////////////////////////////////

    system_status_t status;
    system_status_reset(&status, wd_reboot);

    //////////////////////////////////////////////////////
    // Main loop
    //////////////////////////////////////////////////////

    bool led_state = false;

    while (true)
    {
        watchdog_update();

        uint32_t now_ms = to_ms_since_boot(get_absolute_time());

        //////////////////////////////////////////////////////
        // Periodic OLED reinit
        //////////////////////////////////////////////////////

        if ((now_ms - status.last_reinit_ms) >= OLED_REINIT_PERIOD_MS)
        {
            mission_oled_init_all();
            status.last_reinit_ms = now_ms;
        }

        //////////////////////////////////////////////////////
        // Validate state and recover if needed
        //////////////////////////////////////////////////////

        if (!system_status_is_valid(&status))
        {
            system_status_reset(&status, wd_reboot);
        }

        //////////////////////////////////////////////////////
        // Update state machine
        //////////////////////////////////////////////////////

        update_state_machine(&status);

        //////////////////////////////////////////////////////
        // OLED 1: animated state machine diagram
        //////////////////////////////////////////////////////

        ssd1306_set_i2c_port(I2C_PORT_OLED_1);
        draw_state_machine_screen(&status);

        watchdog_update();

        //////////////////////////////////////////////////////
        // OLED 2: safety parameters
        //////////////////////////////////////////////////////

        ssd1306_set_i2c_port(I2C_PORT_OLED_2);
        draw_safety_screen(&status);

        watchdog_update();

        //////////////////////////////////////////////////////
        // Heartbeat LED
        //////////////////////////////////////////////////////

        led_state = !led_state;
        gpio_put(LED_PIN, led_state);

        //////////////////////////////////////////////////////
        // Deterministic delay
        //////////////////////////////////////////////////////

        mission_delay_ms(FRAME_PERIOD_MS);
    }
}