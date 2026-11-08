#include "controls.h"
#include "game_config.h"
#include "platform.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    int position = 100, fraction = 0;
    controls_move_paddle(&position, &fraction, 2);
    assert(position == 100);
    controls_move_paddle(&position, &fraction, -40);
    assert(position == 115);
    position = SCREEN_W - PADDLE_W - 1;
    controls_move_paddle(&position, &fraction, -100);
    assert(position == SCREEN_W - PADDLE_W && fraction == 0);

    int speed = 5;
    bool ready = true;
    uint32_t last = 0;
    controls_adjust_speed(&speed, &ready, &last, -50, PLATFORM_CYCLES_PER_SECOND);
    assert(speed == 6 && !ready);
    controls_adjust_speed(&speed, &ready, &last, -50, 2 * PLATFORM_CYCLES_PER_SECOND);
    assert(speed == 6);
    controls_adjust_speed(&speed, &ready, &last, 0, 2 * PLATFORM_CYCLES_PER_SECOND);
    controls_adjust_speed(&speed, &ready, &last, 50, 2 * PLATFORM_CYCLES_PER_SECOND);
    assert(speed == 5);
    controls_adjust_speed(&speed, &ready, &last, 0, 3 * PLATFORM_CYCLES_PER_SECOND);
    controls_adjust_speed(&speed, &ready, &last, 50, 3 * PLATFORM_CYCLES_PER_SECOND);
    assert(speed == 5);
    puts("Paddle and speed gesture checks passed.");
}
