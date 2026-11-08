#pragma once
#include "game_config.h"
#include <stdint.h>

typedef struct {
    bool bricks[ROWS][COLS];
    int remaining, lives, paddle, bonus_brick, paddle_fraction;
    float ball_x, ball_y, vx, vy;
    int speed_tenths;
    bool speed_gesture_ready, playing, finished, previous_button;
    uint32_t last_speed_gesture, last_fire;
    bool shot_active[SHOT_COUNT];
    int shot_x[SHOT_COUNT], shot_y[SHOT_COUNT];
} GameState;

/* Input is already debounced; one step advances the game by 10 ms. */
void game_state_init(GameState *state, uint32_t seed, uint32_t now, bool button_pressed);
void game_state_step(GameState *state, int paddle_rate, int speed_rate, bool pressed, uint32_t now);
