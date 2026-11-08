#include "game.h"
#include "platform.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>

static uint32_t cycles;
static jmp_buf done;
static int blue_bricks, fireballs, gyro_reads;

uint32_t platform_cycles(void)
{
    cycles += 10000;
    if (cycles > 18000000u) longjmp(done, 1);
    return cycles;
}
uint32_t platform_seed(void) { return 12345; }
bool platform_button_pressed(void)
{
    return (cycles >= 1000000u && cycles < 2000000u) ||
           (cycles >= 3500000u && cycles < 14000000u);
}
bool platform_gyro_rates(int rates[3])
{
    ++gyro_reads;
    rates[0] = rates[1] = rates[2] = 0;
    return true;
}
void platform_rect(unsigned x, unsigned y, unsigned width, unsigned height, uint16_t color)
{
    (void)x;
    if (color == 0x001f && width == 38 && height == 12) ++blue_bricks;
    if (color == 0xf800 && width == 10 && height == 10 && y < 214) ++fireballs;
}
void platform_delay_ms(unsigned milliseconds) { cycles += milliseconds * 16000u; }
[[noreturn]] void platform_sensor_error(void) { assert(!"sensor error"); for (;;) {} }

int main(void)
{
    if (!setjmp(done)) game_run();
    assert(blue_bricks > 0);
    assert(gyro_reads > 10);
    assert(fireballs > 0);
    puts("Game initializes, launches, and fires on a simulated platform.");
}
