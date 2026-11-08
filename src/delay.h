#pragma once

/* Blocking SysTick delay; assumes a 16 MHz core clock and exclusive SysTick use. */
void delay_ms(unsigned milliseconds);
