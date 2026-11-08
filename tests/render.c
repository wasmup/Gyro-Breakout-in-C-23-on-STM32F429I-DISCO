#include "render.h"
#include "platform.h"
#include <assert.h>
#include <stdio.h>

static uint16_t pixels[SCREEN_H][SCREEN_W];
void platform_rect(unsigned x, unsigned y, unsigned width, unsigned height, uint16_t color)
{
    for (unsigned row = y; row < y + height && row < SCREEN_H; ++row)
        for (unsigned col = x; col < x + width && col < SCREEN_W; ++col)
            pixels[row][col] = color;
}
int main(void)
{
    render_win_message();
    assert(pixels[109][89] == GREEN); /* First pixel of Y. */
    assert(pixels[109][95] == BLACK); /* Gap inside Y. */
    render_rect(0, 0, SCREEN_W, SCREEN_H, BLACK);
    render_game_over_message();
    assert(pixels[109][71] == RED); /* First pixel of M. */
    assert(pixels[109][73] == BLACK); /* Gap inside M. */
    puts("Win and loss messages render expected pixels.");
}
