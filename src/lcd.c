#include "main.h"
#include "delay.h"
#include "lcd.h"
#include <stdint.h>

#define GPIO(n) (0x40020000u + (n) * 0x400u)
#define RCC 0x40023800u
#define LTDC 0x40016800u
#define LAYER1 (LTDC + 0x84u)

static uint16_t framebuffer[320][240];

static void pin_mode(unsigned port, unsigned pin, unsigned mode, unsigned af)
{
    uintptr_t base = GPIO(port);
    REG(base) = (REG(base) & ~(3u << (pin * 2))) | (mode << (pin * 2));
    REG(base + 8) |= 3u << (pin * 2);
    REG(base + 0x0c) &= ~(3u << (pin * 2));
    if (mode == 2)
    {
        uintptr_t afr = base + 0x20 + (pin / 8) * 4;
        REG(afr) = (REG(afr) & ~(15u << ((pin % 8) * 4))) | (af << ((pin % 8) * 4));
    }
}

static bool lcd_ok = true;

static bool wait_register(uintptr_t address, uint32_t mask, bool set)
{
    for (unsigned remaining = 100000; remaining; --remaining)
        if (((REG(address) & mask) != 0) == set) return true;
    return false;
}

static void send_byte(uint8_t value, bool data)
{
    if (!lcd_ok) return;
    REG(GPIO(3) + 0x18) = 1u << (13 + (data ? 0 : 16));
    REG(GPIO(2) + 0x18) = 1u << (2 + 16);
    if (!wait_register(SPI5 + 8, 2u, true)) {
        lcd_ok = false;
        REG(GPIO(2) + 0x18) = 1u << 2;
        return;
    }
    *(volatile uint8_t *)(uintptr_t)(SPI5 + 0x0c) = value;
    if (!wait_register(SPI5 + 8, 2u, true) ||
        !wait_register(SPI5 + 8, 1u << 7, false)) {
        lcd_ok = false;
        REG(GPIO(2) + 0x18) = 1u << 2;
        return;
    }
    REG(GPIO(2) + 0x18) = 1u << 2;
}

static void command(uint8_t cmd, const uint8_t *bytes, unsigned count)
{
    send_byte(cmd, false);
    for (unsigned i = 0; i < count; ++i)
        send_byte(bytes[i], true);
}
#define CMD(c, ...)                        \
    do                                     \
    {                                      \
        const uint8_t b[] = {__VA_ARGS__}; \
        command(c, b, sizeof b);           \
    } while (0)

bool lcd_init(void)
{
    lcd_ok = true;
    REG(RCC + 0x30) |= 0x6fu;                   /* GPIO A, B, C, D, F, G */
    REG(RCC + 0x44) |= (1u << 20) | (1u << 26); /* SPI5, LTDC */
    (void)REG(RCC + 0x44);
    pin_mode(2, 2, 1, 0);
    pin_mode(3, 13, 1, 0);
    REG(GPIO(2) + 0x18) = 1u << 2;
    pin_mode(5, 7, 2, 5);
    pin_mode(5, 9, 2, 5);
    REG(SPI5) = (1u << 2) | (1u << 8) | (1u << 9) | (1u << 3) | (1u << 6); /* master, /4 */
    delay_ms(50);
    CMD(0xca, 0xc3, 0x08, 0x50);
    CMD(0xcf, 0x00, 0xc1, 0x30);
    CMD(0xed, 0x64, 0x03, 0x12, 0x81);
    CMD(0xe8, 0x85, 0x00, 0x78);
    CMD(0xcb, 0x39, 0x2c, 0x00, 0x34, 0x02);
    CMD(0xf7, 0x20);
    CMD(0xea, 0x00, 0x00);
    CMD(0xb1, 0x00, 0x1b);
    CMD(0xb6, 0x0a, 0xa2);
    CMD(0xc0, 0x10);
    CMD(0xc1, 0x10);
    CMD(0xc5, 0x45, 0x15);
    CMD(0xc7, 0x90);
    CMD(0x36, 0xc8);
    CMD(0xf2, 0x00);
    CMD(0xb0, 0xc2);
    CMD(0xb6, 0x0a, 0xa7, 0x27, 0x04);
    CMD(0x2a, 0, 0, 0, 0xef);
    CMD(0x2b, 0, 0, 1, 0x3f);
    CMD(0xf6, 1, 0, 6);
    CMD(0x26, 1);
    CMD(0xe0, 0x0f, 0x29, 0x24, 0x0c, 0x0e, 0x09, 0x4e, 0x78, 0x3c, 0x09, 0x13, 0x05, 0x17, 0x11, 0x00);
    CMD(0xe1, 0x00, 0x16, 0x1b, 0x04, 0x11, 0x07, 0x31, 0x33, 0x42, 0x05, 0x0c, 0x0a, 0x28, 0x2f, 0x0f);
    command(0x11, nullptr, 0);
    delay_ms(200);
    command(0x29, nullptr, 0);
    if (!lcd_ok) return false;

    static const uint8_t pins[][3] = {
        {0, 3, 14}, {0, 4, 14}, {0, 6, 14}, {0, 11, 14}, {0, 12, 14}, {1, 0, 9}, {1, 1, 9}, {1, 8, 14}, {1, 9, 14}, {1, 10, 14}, {1, 11, 14}, {2, 6, 14}, {2, 7, 14}, {2, 10, 14}, {3, 3, 14}, {3, 6, 14}, {5, 10, 14}, {6, 6, 14}, {6, 7, 14}, {6, 10, 9}, {6, 11, 14}, {6, 12, 9}};
    for (unsigned i = 0; i < sizeof pins / sizeof pins[0]; ++i)
        pin_mode(pins[i][0], pins[i][1], 2, pins[i][2]);

    /* PLLSAI: HSI/16 * 192 / 4 / 8 = 6 MHz pixel clock. */
    REG(RCC + 0x04) = (REG(RCC + 0x04) & ~63u) | 16u;
    REG(RCC + 0x88) = (192u << 6) | (7u << 24) | (4u << 28);
    REG(RCC + 0x8c) = (REG(RCC + 0x8c) & ~(3u << 16)) | (2u << 16);
    REG(RCC) |= 1u << 28;
    if (!wait_register(RCC, 1u << 29, true)) return false;

    REG(LTDC + 0x08) = (9u << 16) | 1u;
    REG(LTDC + 0x0c) = (29u << 16) | 3u;
    REG(LTDC + 0x10) = (269u << 16) | 323u;
    REG(LTDC + 0x14) = (279u << 16) | 327u;
    REG(LAYER1 + 0x04) = (269u << 16) | 30u;
    REG(LAYER1 + 0x08) = (323u << 16) | 4u;
    REG(LAYER1 + 0x10) = 2u; /* RGB565 */
    REG(LAYER1 + 0x14) = 255u;
    REG(LAYER1 + 0x1c) = (4u << 8) | 5u;
    REG(LAYER1 + 0x28) = (uintptr_t)framebuffer;
    REG(LAYER1 + 0x2c) = (480u << 16) | 483u;
    REG(LAYER1 + 0x30) = 320u;
    REG(LAYER1) = 1u;
    REG(LTDC + 0x24) = 1u; /* reload shadow registers */
    REG(LTDC + 0x18) |= 1u;
    return true;
}

static bool landscape_mode;
void lcd_set_landscape(bool landscape) { landscape_mode = landscape; }

void lcd_rect(unsigned x, unsigned y, unsigned w, unsigned h, uint16_t color)
{
    unsigned width = landscape_mode ? 320 : 240;
    unsigned height = landscape_mode ? 240 : 320;
    if (x >= width || y >= height)
        return;
    if (w > width - x)
        w = width - x;
    if (h > height - y)
        h = height - y;
    if (landscape_mode)
    {
        for (unsigned col = x; col < x + w; ++col)
            for (unsigned row = y; row < y + h; ++row)
                framebuffer[319 - col][row] = color;
    }
    else
    {
        for (unsigned row = y; row < y + h; ++row)
            for (unsigned col = x; col < x + w; ++col)
                framebuffer[row][col] = color;
    }
}
