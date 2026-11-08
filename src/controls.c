#include "controls.h"
#include "game_config.h"
#include "platform.h"

static int clamp(int value, int minimum, int maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

void controls_move_paddle(int *position, int *fraction, int rate)
{
    if (rate >= -2 && rate <= 2) rate = 0;
    int gain = clamp(rate, -100, 100);
    if (gain > 20) gain += gain - 20;
    else if (gain < -20) gain += gain + 20;
    *fraction -= gain;
    *position = clamp(*position + *fraction / 4, 0, SCREEN_W - PADDLE_W);
    *fraction %= 4;
    if (*position == 0 || *position == SCREEN_W - PADDLE_W) *fraction = 0;
}

void controls_adjust_speed(int *speed, bool *ready, uint32_t *last, int rate, uint32_t now)
{
    if (rate >= -10 && rate <= 10) *ready = true;
    if (*ready && (rate >= 40 || rate <= -40)) {
        *ready = false;
        if (now - *last >= PLATFORM_CYCLES_PER_SECOND) {
            *speed = clamp(*speed + (rate < 0 ? 1 : -1), 5, 13);
            *last = now;
        }
    }
}
