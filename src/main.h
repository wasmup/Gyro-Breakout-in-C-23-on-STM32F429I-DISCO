#pragma once

#include <stdint.h>

/* Shared memory-mapped register access and SPI5 peripheral base. */
#define REG(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define SPI5 0x40015000u
