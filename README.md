# Gyro Breakout in C 23 on STM32F429I-DISCO

A bare-metal Breakout game for the **STM32F429I-DISCO** development board. Rotate the board to steer the paddle, clear the bricks with the ball and fireballs, and try to keep all five lives.

The game uses the board's built-in LCD, gyroscope, blue user button, and ST-LINK. No external components or operating system are required.

## How to play

1. Connect power and keep the board still during startup. The gyroscope calibrates for approximately one second.
2. Rotate the board left or right to move the paddle. The gyroscope measures **rotation rate**: stop rotating and the paddle stops, even if the board remains tilted.
3. Press the blue user button to launch the ball. While the ball is in play, press or hold the button to fire red fireballs from both ends of the paddle. Holding it repeats fire approximately every 200 ms.
4. Give the board a quick rotation on the other axis to adjust ball speed in 0.1× steps. Rotate in the opposite direction to change speed the other way. Return the board to rest between gestures; accepted gestures have a one-second cooldown. Speed starts at **0.5×** and ranges from **0.5× to 1.3×**.
5. Clear all 40 bricks to win. You start with five lives, shown as blue indicators at the top of the screen. Hitting the blue bonus brick with the ball adds two lives; a fireball destroys it without the bonus.

After a win or game over, press the blue button to start a new game. Release it and press again to launch the next ball.

## Build and flash

### Requirements

- STM32F429I-DISCO with its onboard ST-LINK connected to your computer
- GNU Make
- An `arm-none-eabi-gcc` toolchain with C23 support
- OpenOCD with the `board/stm32f429discovery.cfg` board configuration

From the repository root:

```sh
make build   # Build build/gyro-breakout.elf without flashing
make flash   # Build if needed, then program, verify, and reset the board
```

You can also run `make` to **clean, build, and flash** in sequence. `make clean` removes the build directory. Set `CROSS_COMPILE` or `OPENOCD` when your toolchain or OpenOCD executable uses a different name.

No external CMSIS library is required by this build.

## Run the host tests

A host C compiler with C23 support is required. Run:

```sh
make test
```

The host tests cover collision calculations, controls, game state transitions, simulated wins and losses, selected rendering output, and a game-loop smoke test. They do not replace testing on the physical board.

## Implementation

The landscape playfield is 320 × 240 pixels, with an 8 × 5 brick grid, a 96 × 7 paddle, and a 10 × 10 ball. The game uses LTDC with a single RGB565 framebuffer in internal RAM. Drawing is performed by the CPU; there is no framebuffer swap synchronized to vertical blanking, so tearing is possible.

Game state and collision logic are separated from hardware access through a small platform interface. The main loop targets a 10 ms update interval, with limited catch-up after delays. The gyroscope driver reads the onboard sensor over SPI5, calibrates its bias at startup, and filters rotation-rate samples.

```text
src/game.c        Game loop, input polling, and frame timing
src/game_state.c  Game state, ball physics, fireballs, and transitions
src/controls.c    Paddle and speed control
src/collision.c   Collision calculations
src/render.c      Game drawing
src/main.c        Startup and STM32 platform interface
src/gyro.c        Gyroscope driver
src/lcd.c         LCD and framebuffer driver
src/delay.c       Blocking SysTick delay
tests/            Host tests
linker.ld         STM32F429 memory layout
```


 # The firmware’s text, data, and BSS sizes

```
text    data     bss     dec     hex filename
6760       8  153772  160540   2731c build/gyro-breakout.elf
```

## Troubleshooting

- **Controls drift or behave unexpectedly:** reset the board and keep it still during gyroscope calibration. The control directions depend on how the board is held.
- **Red LED lit or a solid red screen:** LCD or gyroscope initialization failed, or a gyroscope read failed during play. Reset the board and try again. If the LCD fails to initialize, the red LED provides an independent indication.
- **OpenOCD cannot connect:** check the ST-LINK USB connection and that your OpenOCD installation includes the STM32F429 Discovery board configuration.
