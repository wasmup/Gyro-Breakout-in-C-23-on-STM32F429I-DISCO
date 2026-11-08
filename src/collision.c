#include "collision.h"

bool collision_paddle(float old_x, float old_y, float new_x, float new_y,
                      int size, int paddle_x, int paddle_y, int paddle_width,
                      float *fraction, float *impact_x)
{
    if (new_y <= old_y || old_y + size > paddle_y || new_y + size < paddle_y)
        return false;
    *fraction = (paddle_y - size - old_y) / (new_y - old_y);
    *impact_x = old_x + (new_x - old_x) * *fraction;
    return *impact_x + size > paddle_x && *impact_x < paddle_x + paddle_width;
}

void collision_brick_entry(float old_x, float old_y, float dx, float dy,
                           int size, int x, int y, int width, int height,
                           float *entry_x, float *entry_y)
{
    *entry_x = *entry_y = -1.0f;
    if (dx > 0 && old_x + size <= x) *entry_x = (x - old_x - size) / dx;
    else if (dx < 0 && old_x >= x + width) *entry_x = (x + width - old_x) / dx;
    if (dy > 0 && old_y + size <= y) *entry_y = (y - old_y - size) / dy;
    else if (dy < 0 && old_y >= y + height) *entry_y = (y + height - old_y) / dy;
}
