#pragma once
#include "game_config.h"
#include <stdint.h>

void render_rect(int x, int y, int width, int height, uint16_t color);
void render_erase_old_ball(int old_x, int old_y, int new_x, int new_y);
void render_restore_bricks(bool bricks[ROWS][COLS], int bonus_brick, int x, int y);
void render_speed(int tenths);
void render_win_message(void);
void render_game_over_message(void);
