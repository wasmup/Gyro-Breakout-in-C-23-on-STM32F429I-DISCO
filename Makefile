CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
OPENOCD ?= openocd
HOST_CC ?= cc
BUILD_DIR := build
ELF := $(BUILD_DIR)/gyro-breakout.elf
SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
ARCH_FLAGS := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
CFLAGS := $(ARCH_FLAGS) -std=c23 -Os -g3 -Wall -Wextra -Wpedantic -ffreestanding -ffunction-sections -fdata-sections -MMD -MP
LDFLAGS := $(ARCH_FLAGS) -nostdlib -T linker.ld -Wl,--gc-sections,-Map=$(BUILD_DIR)/gyro-breakout.map

.PHONY: all build flash clean test
all:
	$(MAKE) clean
	$(MAKE) flash
build: $(ELF)
$(ELF): $(OBJECTS) linker.ld
	$(CC) $(LDFLAGS) $(OBJECTS) -lgcc -o $@
$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
flash: build
	$(OPENOCD) -f board/stm32f429discovery.cfg -c "program $(ELF) verify reset exit"
test:
	@mkdir -p $(BUILD_DIR)
	$(HOST_CC) -std=c2x -Wall -Wextra -Wpedantic -Isrc tests/game_smoke.c src/game.c src/game_state.c src/collision.c src/render.c src/controls.c -o $(BUILD_DIR)/game-smoke
	$(BUILD_DIR)/game-smoke
	$(HOST_CC) -std=c2x -Wall -Wextra -Wpedantic -Isrc tests/collision.c src/collision.c -o $(BUILD_DIR)/collision-test
	$(BUILD_DIR)/collision-test
	$(HOST_CC) -std=c2x -Wall -Wextra -Wpedantic -Isrc tests/controls.c src/controls.c -o $(BUILD_DIR)/controls-test
	$(BUILD_DIR)/controls-test
	$(HOST_CC) -std=c2x -Wall -Wextra -Wpedantic -Isrc tests/game_state.c src/game_state.c src/controls.c src/collision.c -o $(BUILD_DIR)/game-state-test
	$(BUILD_DIR)/game-state-test
	$(HOST_CC) -std=c2x -Wall -Wextra -Wpedantic -Isrc tests/render.c src/render.c -o $(BUILD_DIR)/render-test
	$(BUILD_DIR)/render-test
clean:
	rm -rf $(BUILD_DIR)
-include $(OBJECTS:.o=.d)
