#include "main.h"
#include "delay.h"
#include "platform.h"

void delay_ms(unsigned milliseconds)
{
    REG(0xe000e014u) = PLATFORM_CYCLES_PER_SECOND / 1000u - 1u;
    REG(0xe000e018u) = 0;
    REG(0xe000e010u) = 5u;
    while (milliseconds--)
        while (!(REG(0xe000e010u) & (1u << 16)))
        {
        }
    REG(0xe000e010u) = 0;
}
