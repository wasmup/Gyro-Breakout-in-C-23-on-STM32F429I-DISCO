#include "render.h"
#include "game_config.h"
#include "platform.h"

static int clamp(int value, int minimum, int maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

void render_rect(int x, int y, int w, int h, uint16_t color)
{
    if (x < 0 || y < 0)
        return;
    platform_rect((unsigned)x, (unsigned)y, (unsigned)w, (unsigned)h, color);
}
void render_erase_old_ball(int old_x, int old_y, int new_x, int new_y)
{
    /* Erase only the parts of the old square outside the new square. */
    int left = clamp(new_x - old_x, 0, BALL_SIZE);
    int right = clamp(old_x - new_x, 0, BALL_SIZE);
    int top = clamp(new_y - old_y, 0, BALL_SIZE);
    int bottom = clamp(old_y - new_y, 0, BALL_SIZE);
    if (top)
        render_rect(old_x, old_y, BALL_SIZE, top, BLACK);
    if (bottom)
        render_rect(old_x, old_y + BALL_SIZE - bottom, BALL_SIZE, bottom, BLACK);
    if (left)
        render_rect(old_x, old_y + top, left, BALL_SIZE - top - bottom, BLACK);
    if (right)
        render_rect(old_x + BALL_SIZE - right, old_y + top, right, BALL_SIZE - top - bottom, BLACK);
}
/* Restore surviving bricks touched by an erased moving object. */
void render_restore_bricks(bool bricks[ROWS][COLS], int bonus_brick, int x, int y)
{
    if (y + BALL_SIZE <= BRICK_TOP || y >= BRICK_TOP + ROWS * BRICK_PITCH_Y || x >= SCREEN_W)
        return;
    int first_col = clamp(x / BRICK_PITCH_X, 0, COLS - 1);
    int last_col = clamp((x + BALL_SIZE - 1) / BRICK_PITCH_X, 0, COLS - 1);
    int first_row = clamp((y - BRICK_TOP) / BRICK_PITCH_Y, 0, ROWS - 1);
    int last_row = clamp((y + BALL_SIZE - 1 - BRICK_TOP) / BRICK_PITCH_Y, 0, ROWS - 1);
    for (int r = first_row; r <= last_row; ++r)
        for (int c = first_col; c <= last_col; ++c)
            if (bricks[r][c])
                render_rect(c * BRICK_PITCH_X + 1, BRICK_TOP + r * BRICK_PITCH_Y, BRICK_WIDTH, BRICK_HEIGHT, (r * COLS + c == bonus_brick) ? BLUE : GREEN);
}
void render_speed(int tenths)
{
    /* 3x5 glyphs: digits, decimal point, x. */
    static const uint16_t glyphs[] = {
        0x7b6fu, 0x2492u, 0x73e7u, 0x73cfu, 0x5bc9u,
        0x79cfu, 0x79efu, 0x7249u, 0x7befu, 0x7bcfu, 0x0002u, 0x0550u};
    int symbols[] = {tenths / 10, 10, tenths % 10, 11};
    render_rect(SCREEN_W - 36, 3, 34, 10, BLACK);
    for (int i = 0; i < 4; ++i)
    {
        uint16_t bits = glyphs[symbols[i]];
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 3; ++x)
                if (bits & (1u << (14 - (y * 3 + x))))
                    render_rect(SCREEN_W - 36 + i * 8 + x * 2, 3 + y * 2, 2, 2, WHITE);
    }
}
static void draw_glyphs(const uint8_t glyphs[][7], unsigned count,
                        int left, int top, int scale, uint16_t color)
{
    for (unsigned i = 0; i < count; ++i)
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
                if (glyphs[i][y] & (1u << (4 - x)))
                    render_rect(left + (int)i * 6 * scale + x * scale,
                         top + y * scale, scale, scale, color);
}
void render_win_message(void)
{
    /* 5x7 bitmap letters spelling YOU WIN! */
    static const uint8_t glyphs[][7] = {
        {17, 17, 10, 4, 4, 4, 4},
        {14, 17, 17, 17, 17, 17, 14},
        {17, 17, 17, 17, 17, 17, 14},
        {0, 0, 0, 0, 0, 0, 0},
        {17, 17, 17, 21, 21, 21, 10},
        {14, 4, 4, 4, 4, 4, 14},
        {17, 25, 25, 21, 19, 19, 17},
        {4, 4, 4, 4, 4, 0, 4}};
    draw_glyphs(glyphs, 8, 89, 109, 3, GREEN);
}
void render_game_over_message(void)
{
    /* 5x7 bitmap letters spelling MAYBE NEXT TIME. */
    static const uint8_t glyphs[][7] = {
        {17, 27, 21, 21, 17, 17, 17}, /* M */
        {14, 17, 17, 31, 17, 17, 17}, /* A */
        {17, 17, 10, 4, 4, 4, 4},     /* Y */
        {30, 17, 17, 30, 17, 17, 30}, /* B */
        {31, 16, 16, 30, 16, 16, 31}, /* E */
        {0, 0, 0, 0, 0, 0, 0},
        {17, 25, 25, 21, 19, 19, 17}, /* N */
        {31, 16, 16, 30, 16, 16, 31}, /* E */
        {17, 17, 10, 4, 10, 17, 17},  /* X */
        {31, 4, 4, 4, 4, 4, 4},       /* T */
        {0, 0, 0, 0, 0, 0, 0},
        {31, 4, 4, 4, 4, 4, 4},       /* T */
        {14, 4, 4, 4, 4, 4, 14},      /* I */
        {17, 27, 21, 21, 17, 17, 17}, /* M */
        {31, 16, 16, 30, 16, 16, 31}  /* E */
    };
    draw_glyphs(glyphs, 15, 71, 109, 2, RED);
}
