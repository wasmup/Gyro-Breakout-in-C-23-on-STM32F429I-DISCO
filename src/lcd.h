#pragma once
#include <stdint.h>
bool lcd_init(void);
void lcd_rect(unsigned x, unsigned y, unsigned w, unsigned h, uint16_t color);

void lcd_set_landscape(bool landscape);
