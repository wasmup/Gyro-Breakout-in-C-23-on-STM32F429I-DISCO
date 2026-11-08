#include "game.h"
#include "game_state.h"
#include "game_config.h"
#include "render.h"
#include "controls.h"
#include "collision.h"
#include "platform.h"
#include <stdint.h>

#define CORE_CYCLES_PER_SECOND PLATFORM_CYCLES_PER_SECOND
#define FRAME_CYCLES (PLATFORM_CYCLES_PER_SECOND / 100u)
#define FIRE_INTERVAL_CYCLES (PLATFORM_CYCLES_PER_SECOND / 5u)
#define CYCLE_COUNT platform_cycles()

static bool button_raw(void) { return platform_button_pressed(); }

/* Require a stable input for 20 ms before accepting either edge. */
static bool button(void)
{
    static bool stable, candidate;
    static uint32_t changed_at;
    bool raw = button_raw();
    uint32_t now = CYCLE_COUNT;
    if (raw != candidate)
    {
        candidate = raw;
        changed_at = now;
    }
    if (candidate != stable && now - changed_at >= CORE_CYCLES_PER_SECOND / 50u)
        stable = candidate;
    return stable;
}

static void draw_initial(const GameState *s)
{
    platform_rect(0, 0, SCREEN_W, SCREEN_H, BLACK);
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c)
            render_rect(c * BRICK_PITCH_X + 1, BRICK_TOP + r * BRICK_PITCH_Y,
                        BRICK_WIDTH, BRICK_HEIGHT,
                        r * COLS + c == s->bonus_brick ? BLUE : GREEN);
}

static void wait_for_frame(uint32_t *next_frame)
{
    *next_frame += FRAME_CYCLES;
    uint32_t now = CYCLE_COUNT;
    if ((int32_t)(*next_frame - now) > 0) {
        while ((int32_t)(*next_frame - CYCLE_COUNT) > 0) { }
    } else if (now - *next_frame > 3u * FRAME_CYCLES) {
        *next_frame = now - 3u * FRAME_CYCLES;
    }
}

[[noreturn]] void game_run(void)
{
    bool previous_button = button();
    for (;;) {
        GameState state;
        game_state_init(&state, platform_seed(), CYCLE_COUNT, previous_button);
        draw_initial(&state);
        int displayed_speed = -1;
        uint32_t next_frame = CYCLE_COUNT;
        while (!state.finished) {
            int old_x = (int)state.ball_x, old_y = (int)state.ball_y;
            int old_paddle = state.paddle;
            bool old_bricks[ROWS][COLS];
            bool old_shots[SHOT_COUNT];
            int old_shot_x[SHOT_COUNT], old_shot_y[SHOT_COUNT];
            for (int r = 0; r < ROWS; ++r)
                for (int c = 0; c < COLS; ++c) old_bricks[r][c] = state.bricks[r][c];
            for (int i = 0; i < SHOT_COUNT; ++i) {
                old_shots[i] = state.shot_active[i];
                old_shot_x[i] = state.shot_x[i]; old_shot_y[i] = state.shot_y[i];
            }
            int rates[3];
            if (!platform_gyro_rates(rates)) platform_sensor_error();
            game_state_step(&state, rates[0], rates[1], button(), CYCLE_COUNT);
            if (state.speed_tenths != displayed_speed) {
                render_speed(state.speed_tenths);
                displayed_speed = state.speed_tenths;
            }
            for (int r = 0; r < ROWS; ++r)
                for (int c = 0; c < COLS; ++c)
                    if (old_bricks[r][c] && !state.bricks[r][c])
                        render_rect(c * BRICK_PITCH_X + 1, BRICK_TOP + r * BRICK_PITCH_Y,
                                    BRICK_WIDTH, BRICK_HEIGHT, BLACK);
            for (int i = 0; i < SHOT_COUNT; ++i) if (old_shots[i]) {
                render_rect(old_shot_x[i], old_shot_y[i], BALL_SIZE, BALL_SIZE, BLACK);
                render_restore_bricks(state.bricks, state.bonus_brick, old_shot_x[i], old_shot_y[i]);
            }
            render_erase_old_ball(old_x, old_y, (int)state.ball_x, (int)state.ball_y);
            render_restore_bricks(state.bricks, state.bonus_brick, old_x, old_y);
            if (state.paddle > old_paddle)
                render_rect(old_paddle, PADDLE_Y, state.paddle - old_paddle, 7, BLACK);
            else if (state.paddle < old_paddle)
                render_rect(state.paddle + PADDLE_W, PADDLE_Y, old_paddle - state.paddle, 7, BLACK);
            for (int i = 0; i < LIVES_MAX; ++i)
                render_rect(5 + i * 14, 4, 10, 8, i < state.lives ? BLUE : BLACK);
            render_rect(state.paddle, PADDLE_Y, PADDLE_W, 7, WHITE);
            if (state.ball_y < SCREEN_H)
                render_rect((int)state.ball_x, (int)state.ball_y, BALL_SIZE, BALL_SIZE, GREEN);
            for (int i = 0; i < SHOT_COUNT; ++i)
                if (state.shot_active[i])
                    render_rect(state.shot_x[i], state.shot_y[i], BALL_SIZE, BALL_SIZE, RED);
            wait_for_frame(&next_frame);
        }
        render_rect(SCREEN_W / 2 - 5, 4, 10, 8, state.remaining ? RED : GREEN);
        if (!state.remaining) render_win_message();
        else render_game_over_message();
        do { previous_button = button(); platform_delay_ms(20); } while (previous_button);
        do { previous_button = button(); platform_delay_ms(20); } while (!previous_button);
    }
}
