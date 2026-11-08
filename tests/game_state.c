#include "game_state.h"
#include "platform.h"
#include <assert.h>
#include <stdio.h>

static void keep_one_brick(GameState *s, int row, int col)
{
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c) s->bricks[r][c] = false;
    s->bricks[row][col] = true;
    s->remaining = 1;
}

static void prepare_hit(GameState *s)
{
    s->playing = true;
    s->ball_x = 2; s->ball_y = BRICK_TOP + 4 * BRICK_PITCH_Y + BRICK_HEIGHT + 1;
    s->vx = 0; s->vy = -5;
}

int main(void)
{
    GameState s;
    game_state_init(&s, 3, 0, false);
    assert(s.lives == 5 && s.remaining == 40 && !s.playing && !s.finished);
    game_state_step(&s, 0, 0, true, 160000);
    assert(s.playing && !s.shot_active[0]);
    game_state_step(&s, 0, 0, true, 160000 + PLATFORM_CYCLES_PER_SECOND / 5);
    assert(s.shot_active[0] && s.shot_active[1]);

    game_state_init(&s, 7, 0, false);
    keep_one_brick(&s, 4, 0);
    s.bonus_brick = 4 * COLS;
    prepare_hit(&s);
    game_state_step(&s, 0, 0, false, 160000);
    assert(s.lives == 7 && s.remaining == 0 && s.finished && s.vy > 0);

    game_state_init(&s, 7, 0, false);
    keep_one_brick(&s, 4, 0);
    s.bonus_brick = 4 * COLS;
    s.playing = true; s.ball_x = 150; s.ball_y = 180;
    s.shot_active[0] = true; s.shot_x[0] = 2; s.shot_y[0] = 110;
    game_state_step(&s, 0, 0, false, 160000);
    assert(s.lives == 5 && s.remaining == 0 && s.finished && !s.shot_active[0]);

    game_state_init(&s, 11, 0, false);
    keep_one_brick(&s, 4, 0);
    s.bricks[0][0] = true; ++s.remaining;
    s.bonus_brick = 1;
    prepare_hit(&s);
    game_state_step(&s, 0, 0, false, 160000);
    assert(s.remaining == 1 && !s.finished && !s.bricks[4][0]);
    s.ball_y = BRICK_TOP + BRICK_HEIGHT + 1; s.ball_x = 2; s.vx = 0; s.vy = -5;
    game_state_step(&s, 0, 0, false, 320000);
    assert(s.remaining == 0 && s.finished && !s.bricks[0][0]);

    game_state_init(&s, 7, 0, false);
    s.lives = 1; s.playing = true; s.ball_x = 300; s.ball_y = 239;
    s.vx = 0; s.vy = 5;
    game_state_step(&s, 0, 0, false, 160000);
    assert(s.lives == 0 && s.finished && !s.playing);
    game_state_init(&s, 9, 1000000, true);
    assert(s.lives == 5 && s.remaining == 40 && !s.finished && !s.playing);
    game_state_step(&s, 0, 0, true, 1160000);
    assert(!s.playing); /* Restart press must be released before launch. */
    game_state_step(&s, 0, 0, false, 1320000);
    game_state_step(&s, 0, 0, true, 1480000);
    assert(s.playing);
    /* Play a full level with held fire and regular paddle sweeps. */
    game_state_init(&s, 123, 0, false);
    uint32_t now = 0;
    int target = 0;
    for (int i = 0; i < 15000 && !s.finished; ++i) {
        now += 160000;
        if (i % 100 == 0) target = (target + 31) % 225;
        int rate = s.paddle < target - 3 ? -45 : s.paddle > target + 3 ? 45 : 0;
        bool press = s.playing || !s.previous_button;
        game_state_step(&s, rate, 0, press, now);
    }
    assert(s.finished && s.remaining == 0 && s.lives > 0);

    /* Play again without firing, steering away from the ball to lose lives. */
    game_state_init(&s, 123, 0, false);
    now = 0;
    for (int i = 0; i < 100000 && !s.finished; ++i) {
        now += 160000;
        int away = s.ball_x < 160 ? 224 : 0;
        int rate = s.paddle < away - 3 ? -45 : s.paddle > away + 3 ? 45 : 0;
        bool press = !s.playing && !s.previous_button;
        game_state_step(&s, rate, 0, press, now);
    }
    assert(s.finished && s.lives == 0 && s.remaining > 0);
    puts("Game state launch, fire, bonus, win, loss, and restart checks passed.");
}
