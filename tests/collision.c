#include "collision.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    float fraction, impact, x_entry, y_entry;
    /* The end position overlaps, but the earlier impact point misses. */
    assert(!collision_paddle(88, 203, 91, 205, 10, 100, 214, 96, &fraction, &impact));
    /* The end position misses, but the impact point overlaps. */
    assert(collision_paddle(90, 203, 87, 205, 10, 100, 214, 96, &fraction, &impact) == false);
    assert(collision_paddle(91.5f, 203.2f, 88.5f, 205.2f, 10, 100, 214, 96, &fraction, &impact));
    assert(impact > 90 && impact < 91.5f);
    collision_brick_entry(28.8f, 29.8f, 2, 2, 10, 40, 40, 38, 12, &x_entry, &y_entry);
    assert(x_entry > y_entry);
    collision_brick_entry(29.8f, 28.8f, 2, 2, 10, 40, 40, 38, 12, &x_entry, &y_entry);
    assert(y_entry > x_entry);
    puts("Paddle impact and brick entry checks passed.");
}
