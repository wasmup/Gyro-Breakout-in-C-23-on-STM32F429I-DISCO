#pragma once
#include <stdint.h>

#define PLATFORM_CYCLES_PER_SECOND 16000000u

/* Hardware services used by the game. Time is measured in 16 MHz cycles. */
uint32_t platform_cycles(void);
uint32_t platform_seed(void);
bool platform_button_pressed(void);
bool platform_gyro_rates(int rates[3]);
void platform_rect(unsigned x, unsigned y, unsigned width, unsigned height, uint16_t color);
void platform_delay_ms(unsigned milliseconds);
[[noreturn]] void platform_sensor_error(void);
