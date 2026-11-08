#pragma once
#include <stdint.h>

void controls_move_paddle(int *position, int *fraction, int rate);
void controls_adjust_speed(int *speed_tenths, bool *ready, uint32_t *last_gesture,
                           int rate, uint32_t now);
