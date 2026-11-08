#pragma once

/* Return whether the moving ball crosses the paddle's top within this step. */
bool collision_paddle(float old_x, float old_y, float new_x, float new_y,
                      int ball_size, int paddle_x, int paddle_y, int paddle_width,
                      float *fraction, float *impact_x);

/* Entry time of an overlapping ball into a brick; -1 means an axis was already overlapping. */
void collision_brick_entry(float old_x, float old_y, float dx, float dy,
                           int ball_size, int x, int y, int width, int height,
                           float *entry_x, float *entry_y);
