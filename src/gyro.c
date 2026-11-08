#include "main.h"
#include "delay.h"
#include "gyro.h"
#include <stdint.h>

#define PC 0x40020800u
#define PF 0x40021400u

static bool transfer(uint8_t output, uint8_t *input)
{
    unsigned timeout = 100000;
    while (!(REG(SPI5 + 8) & 2u))
        if (!--timeout)
            return false;
    *(volatile uint8_t *)(uintptr_t)(SPI5 + 0x0c) = output;
    timeout = 100000;
    while (!(REG(SPI5 + 8) & 1u))
        if (!--timeout)
            return false;
    *input = *(volatile uint8_t *)(uintptr_t)(SPI5 + 0x0c);
    return true;
}
static bool transaction(uint8_t command, uint8_t *data, unsigned count, bool read)
{
    uint8_t ignored;
    REG(PC + 0x18) = 1u << 17;
    bool ok = transfer(command, &ignored);
    for (unsigned i = 0; ok && i < count; ++i)
        if (read)
            ok = transfer(0xff, &data[i]);
        else
            ok = transfer(data[i], &ignored);
    unsigned timeout = 100000;
    while (ok && (REG(SPI5 + 8) & (1u << 7)))
        if (!--timeout)
            ok = false;
    REG(PC + 0x18) = 1u << 1;
    return ok;
}
static bool gyro_init(void)
{
    /* LCD and gyro share SPI5. PC1 is gyro CS, PC2 is LCD CS. */
    REG(PC + 0x18) = (1u << 1) | (1u << 2);
    REG(PC) = (REG(PC) & ~(3u << 2)) | (1u << 2);
    REG(PC + 4) &= ~(1u << 1);
    REG(PF) = (REG(PF) & ~(3u << 16)) | (2u << 16);
    REG(PF + 0x24) = (REG(PF + 0x24) & ~15u) | 5u; /* PF8 SPI5 MISO */
    /* Clear any receive overrun left by LCD setup. */
    (void)REG(SPI5 + 0x0c);
    (void)REG(SPI5 + 8);
    uint8_t id = 0;
    if (!transaction(0x8f, &id, 1, true) || (id != 0xd4 && id != 0xd7))
        return false;
    uint8_t control = 0x0f; /* 95 Hz, normal mode, XYZ enabled */
    if (!transaction(0x20, &control, 1, false))
        return false;
    control = 0x80; /* Block updates while reading, +/-250 degrees/s */
    return transaction(0x23, &control, 1, false);
}
static bool read_axes(int16_t axes[3])
{
    uint8_t data[6];
    if (!transaction(0xe8, data, 6, true))
        return false;
    for (unsigned i = 0; i < 3; ++i)
        axes[i] = (int16_t)((uint16_t)data[2 * i] | (uint16_t)data[2 * i + 1] << 8);
    return true;
}
static int32_t offsets[3], history[3][10], totals[3];
static uint32_t calibration_seed;
static unsigned history_index, history_count, history_window = 10;

bool gyro_start(void)
{
    if (!gyro_init())
        return false;
    delay_ms(200);
    int16_t axes[3];
    for (unsigned j = 0; j < 3; ++j)
    {
        offsets[j] = totals[j] = 0;
        for (unsigned i = 0; i < 10; ++i)
            history[j][i] = 0;
    }
    history_index = history_count = 0;
    calibration_seed = 2166136261u;
    for (unsigned i = 0; i < 32; ++i)
    {
        if (!read_axes(axes))
            return false;
        for (unsigned j = 0; j < 3; ++j)
        {
            offsets[j] += axes[j];
            calibration_seed = (calibration_seed ^ (uint16_t)axes[j]) * 16777619u;
        }
        delay_ms(12);
    }
    for (unsigned j = 0; j < 3; ++j)
        offsets[j] /= 32;
    return true;
}

void gyro_set_filter_window(unsigned samples)
{
    history_window = samples < 1 ? 1 : samples > 10 ? 10
                                                    : samples;
    history_index = history_count = 0;
    for (unsigned i = 0; i < 3; ++i)
    {
        totals[i] = 0;
        for (unsigned j = 0; j < 10; ++j)
            history[i][j] = 0;
    }
}

bool gyro_rates(int rates[3])
{
    int16_t axes[3];
    if (!read_axes(axes))
        return false;
    if (history_count < history_window)
        ++history_count;
    for (unsigned i = 0; i < 3; ++i)
    {
        totals[i] -= history[i][history_index];
        history[i][history_index] = (int32_t)axes[i] - offsets[i];
        totals[i] += history[i][history_index];
        int rate = (totals[i] / (int32_t)history_count) * 875 / 100000;
        rates[i] = rate < -250 ? -250 : rate > 250 ? 250
                                                   : rate;
    }
    history_index = (history_index + 1) % history_window;
    return true;
}

uint32_t gyro_calibration_seed(void)
{
    return calibration_seed;
}
