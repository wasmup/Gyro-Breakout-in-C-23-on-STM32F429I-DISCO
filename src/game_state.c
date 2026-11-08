#include "game_state.h"
#include "collision.h"
#include "controls.h"
#include "platform.h"

#define FIRE_INTERVAL_CYCLES (PLATFORM_CYCLES_PER_SECOND / 5u)

static int clamp(int value, int minimum, int maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}
static uint32_t mix_random(uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    return value ^ (value >> 16);
}
void game_state_init(GameState *s, uint32_t seed, uint32_t now, bool button_pressed)
{
    /* Explicit initialization avoids a dependency on libc in the firmware. */
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c) s->bricks[r][c] = true;
    s->remaining = ROWS * COLS; s->lives = LIVES_INITIAL;
    s->paddle = (SCREEN_W - PADDLE_W) / 2; s->paddle_fraction = 0;
    s->bonus_brick = (int)(mix_random(now ^ seed) % (ROWS * COLS));
    s->ball_x = (SCREEN_W - BALL_SIZE) / 2; s->ball_y = PADDLE_Y - BALL_SIZE;
    s->vx = 3.0f; s->vy = -5.0f; s->speed_tenths = 5;
    s->speed_gesture_ready = true; s->playing = s->finished = false;
    s->previous_button = button_pressed;
    s->last_speed_gesture = now - PLATFORM_CYCLES_PER_SECOND; s->last_fire = now;
    for (int i = 0; i < SHOT_COUNT; ++i) {
        s->shot_active[i] = false; s->shot_x[i] = 0; s->shot_y[i] = 0;
    }
}

static void update_controls(GameState *s, int paddle_rate, int speed_rate, bool pressed, uint32_t now)
{
    controls_move_paddle(&s->paddle, &s->paddle_fraction, paddle_rate);
    controls_adjust_speed(&s->speed_tenths, &s->speed_gesture_ready,
                          &s->last_speed_gesture, speed_rate, now);
    if (pressed && !s->previous_button && !s->playing)
    {
        s->playing = true;
        s->last_fire = now;
    }
    else if (pressed && s->playing &&
             (!s->previous_button || now - s->last_fire >= FIRE_INTERVAL_CYCLES))
    {
        for (int i = 0; i < SHOT_COUNT; i += 2)
        {
            if (!s->shot_active[i] && !s->shot_active[i + 1])
            {
                s->shot_x[i] = s->paddle;
                s->shot_x[i + 1] = s->paddle + PADDLE_W - BALL_SIZE;
                s->shot_y[i] = s->shot_y[i + 1] = PADDLE_Y - BALL_SIZE;
                s->shot_active[i] = s->shot_active[i + 1] = true;
                s->last_fire = now;
                break;
            }
        }
    }
    s->previous_button = pressed;
}

/* Resolve successive brick hits along the remainder of one movement step. */
static void resolve_brick_collisions(GameState *s, float previous_x, float previous_y,
                                     int *brick_hits)
{
    while ((*brick_hits) < 4)
    {
    float dx = s->ball_x - previous_x, dy = s->ball_y - previous_y;
    float first_hit = 2.0f, hit_entry_x = -1.0f, hit_entry_y = -1.0f;
    int hit_row = -1, hit_col = -1;
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c)
        {
            int x = c * BRICK_PITCH_X + 1, y = BRICK_TOP + r * BRICK_PITCH_Y;
            if (!s->bricks[r][c] || s->ball_x + BALL_SIZE <= x || s->ball_x >= x + BRICK_WIDTH ||
                s->ball_y + BALL_SIZE <= y || s->ball_y >= y + BRICK_HEIGHT)
                continue;
            float entry_x, entry_y;
            collision_brick_entry(previous_x, previous_y, dx, dy, BALL_SIZE,
                                  x, y, BRICK_WIDTH, BRICK_HEIGHT, &entry_x, &entry_y);
            float entry = entry_x > entry_y ? entry_x : entry_y;
            if (entry < 0)
                entry = 0;
            if (entry < first_hit)
            {
                first_hit = entry;
                hit_row = r;
                hit_col = c;
                hit_entry_x = entry_x;
                hit_entry_y = entry_y;
            }
        }
    if (hit_row < 0) break;
    {
        float impact_x = previous_x + dx * first_hit;
        float impact_y = previous_y + dy * first_hit;
        s->bricks[hit_row][hit_col] = false;
        --s->remaining;
        if (hit_row * COLS + hit_col == s->bonus_brick)
            s->lives += 2;
        if (hit_entry_x >= 0 && hit_entry_x > hit_entry_y)
        {
            s->ball_x = previous_x + dx * (2.0f * hit_entry_x - 1.0f);
            s->vx = -s->vx;
        }
        else if (hit_entry_y >= 0)
        {
            s->ball_y = previous_y + dy * (2.0f * hit_entry_y - 1.0f);
            s->vy = -s->vy;
        }
        else
        {
            s->ball_y = previous_y;
            s->vy = -s->vy;
        }
        ++(*brick_hits);
        previous_x = impact_x;
        previous_y = impact_y;
        if (s->ball_x < 0) { s->ball_x = -s->ball_x; s->vx = -s->vx; }
        if (s->ball_x > SCREEN_W - BALL_SIZE) {
            s->ball_x = 2 * (SCREEN_W - BALL_SIZE) - s->ball_x; s->vx = -s->vx;
        }
        if (s->ball_y < 18) { s->ball_y = 36 - s->ball_y; s->vy = -s->vy; }
    }
    }
}

static void update_ball(GameState *s)
{
    if (!s->playing)
    {
        s->ball_x = s->paddle + (PADDLE_W - BALL_SIZE) / 2;
        s->ball_y = PADDLE_Y - BALL_SIZE;
    }
    else
    {
        float speed_scale = s->speed_tenths * 0.1f;
        /* Small steps prevent the ball from skipping narrow brick edges. */
        float max_velocity = (s->vx < 0 ? -s->vx : s->vx);
        if ((s->vy < 0 ? -s->vy : s->vy) > max_velocity)
            max_velocity = s->vy < 0 ? -s->vy : s->vy;
        int substeps = (int)(max_velocity * speed_scale / 2.0f) + 1;
        int brick_hits = 0;
        for (int step = 0; step < substeps && brick_hits < 4; ++step)
        {
            float previous_x = s->ball_x, previous_y = s->ball_y;
            s->ball_x += s->vx * speed_scale / substeps;
            s->ball_y += s->vy * speed_scale / substeps;
            if (s->ball_x <= 0)
            {
                s->ball_x = -s->ball_x;
                s->vx = -s->vx;
            }
            if (s->ball_x >= SCREEN_W - BALL_SIZE)
            {
                s->ball_x = 2 * (SCREEN_W - BALL_SIZE) - s->ball_x;
                s->vx = -s->vx;
            }
            if (s->ball_y <= 18)
            {
                s->ball_y = 36 - s->ball_y;
                s->vy = -s->vy;
            }
            float fraction, impact_x;
            if (s->vy > 0 && collision_paddle(previous_x, previous_y, s->ball_x, s->ball_y,
                                           BALL_SIZE, s->paddle, PADDLE_Y, PADDLE_W,
                                           &fraction, &impact_x))
            {
                s->vx = (impact_x + BALL_SIZE / 2 - (s->paddle + PADDLE_W / 2)) / 5.0f;
                s->vy = -5.0f;
                s->ball_x = impact_x + s->vx * speed_scale / substeps * (1.0f - fraction);
                s->ball_y = PADDLE_Y - BALL_SIZE + s->vy * speed_scale / substeps * (1.0f - fraction);
            }
            resolve_brick_collisions(s, previous_x, previous_y, &brick_hits);
        }
        if (s->ball_y >= SCREEN_H)
        {
            --s->lives;
            s->playing = false;
            s->vx = 3.0f;
            s->vy = -5.0f;
            if (!s->lives)
                s->finished = true;
        }
        if (!s->remaining)
            s->finished = true;
    }
}

static void update_shots(GameState *s)
{
    for (int i = 0; i < SHOT_COUNT; ++i)
        if (s->shot_active[i])
        {
            int previous_y = s->shot_y[i];
            s->shot_y[i] -= 8;
            if (!s->playing || s->shot_y[i] < 18)
                s->shot_active[i] = false;
            else
            {
                int first_col = clamp(s->shot_x[i] / BRICK_PITCH_X, 0, COLS - 1);
                int last_col = clamp((s->shot_x[i] + BALL_SIZE - 1) / BRICK_PITCH_X, 0, COLS - 1);
                int first_row = clamp((s->shot_y[i] - BRICK_TOP) / BRICK_PITCH_Y, 0, ROWS - 1);
                int last_row = clamp((previous_y + BALL_SIZE - 1 - BRICK_TOP) / BRICK_PITCH_Y, 0, ROWS - 1);
                for (int r = last_row; r >= first_row && s->shot_active[i]; --r)
                {
                    for (int c = first_col; c <= last_col; ++c)
                    {
                        int x = c * BRICK_PITCH_X + 1, y = BRICK_TOP + r * BRICK_PITCH_Y;
                        if (s->bricks[r][c] && s->shot_x[i] + BALL_SIZE > x && s->shot_x[i] < x + BRICK_WIDTH &&
                            s->shot_y[i] < y + BRICK_HEIGHT && previous_y + BALL_SIZE > y)
                        {
                            s->bricks[r][c] = false;
                            --s->remaining;
                            s->shot_active[i] = false;
                            if (!s->remaining)
                                s->finished = true;
                            break;
                        }
                    }
                }
            }
        }
}

void game_state_step(GameState *s, int paddle_rate, int speed_rate, bool pressed, uint32_t now)
{
    if (s->finished) return;
    update_controls(s, paddle_rate, speed_rate, pressed, now);
    update_ball(s);
    update_shots(s);
}
