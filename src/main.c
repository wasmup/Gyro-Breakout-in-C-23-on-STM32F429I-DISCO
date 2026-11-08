#include "main.h"
#include "delay.h"
#include "game.h"
#include "gyro.h"
#include "lcd.h"
#include "platform.h"

extern uint32_t _estack, _sdata, _edata, _sidata, _sbss, _ebss;
[[noreturn]] void Reset_Handler(void);
__attribute__((section(".isr_vector"), used))
const uintptr_t vectors[] = {(uintptr_t)&_estack, (uintptr_t)&Reset_Handler};

uint32_t platform_cycles(void) { return REG(0xe0001004u); }
uint32_t platform_seed(void) { return gyro_calibration_seed(); }
bool platform_button_pressed(void) { return (REG(0x40020010u) & 1u) != 0; }
bool platform_gyro_rates(int rates[3]) { return gyro_rates(rates); }
void platform_rect(unsigned x, unsigned y, unsigned width, unsigned height, uint16_t color)
{
    lcd_rect(x, y, width, height, color);
}
void platform_delay_ms(unsigned milliseconds) { delay_ms(milliseconds); }
[[noreturn]] void platform_sensor_error(void)
{
    /* PG14 is the Discovery board's red LED, independent of the LCD. */
    REG(0x40023830u) |= 1u << 6;
    (void)REG(0x40023830u);
    REG(0x40021804u) &= ~(1u << 14);
    REG(0x40021800u) = (REG(0x40021800u) & ~(3u << 28)) | (1u << 28);
    REG(0x40021818u) = 1u << 14;
    lcd_rect(0, 0, 320, 240, 0xf800u);
    for (;;) { }
}

static void initialize_board(void)
{
    REG(0x40023830u) |= 1u | (1u << 2);
    (void)REG(0x40023830u);
    REG(0x40020000u) &= ~3u;
    REG(0x4002000cu) &= ~3u;
    REG(0x40020818u) = 1u << 1;
    REG(0x40020804u) &= ~(1u << 1);
    REG(0x40020800u) = (REG(0x40020800u) & ~(3u << 2)) | (1u << 2);
    if (!lcd_init()) platform_sensor_error();
    lcd_set_landscape(true);
    if (!gyro_start())
        platform_sensor_error();
    gyro_set_filter_window(2);
    REG(0xe000edfcu) |= 1u << 24;
    REG(0xe0001004u) = 0;
    REG(0xe0001000u) |= 1u;
}

[[noreturn]] void Reset_Handler(void)
{
    REG(0xe000ed88u) |= 0xfu << 20;
    __asm volatile("dsb\n\tisb" ::: "memory");
    const uint32_t *source = &_sidata;
    for (uint32_t *dest = &_sdata; dest < &_edata; ++dest)
        *dest = *source++;
    for (uint32_t *dest = &_sbss; dest < &_ebss; ++dest)
        *dest = 0;
    initialize_board();
    game_run();
}
