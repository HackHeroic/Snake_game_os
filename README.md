# Snake Game OS

A real-time terminal-based Snake game written entirely in C with custom-built libraries for memory management, string manipulation, math operations, screen rendering, and keyboard input. Built as an OS course capstone project (Track A).

## Features

- Classic snake gameplay with smooth terminal rendering
- Two game modes: Classic (wall collision = death) and Wrap-Around (snake wraps edges)
- Progressive difficulty: speed increases every 5 food eaten
- Pause/Resume with P key
- Persistent high score system
- Gradient-colored snake with game-over animation
- WASD + Arrow key controls

## Building

```bash
make        # build the game
make run    # build and run
make test   # run unit tests
make clean  # remove build artifacts
```

## Requirements

- GCC compiler
- POSIX-compatible terminal (macOS, Linux)
- Terminal size: minimum 22x14 characters

## Controls

| Key | Action |
|-----|--------|
| W / Up Arrow | Move Up |
| A / Left Arrow | Move Left |
| S / Down Arrow | Move Down |
| D / Right Arrow | Move Right |
| P | Pause / Resume |
| Q | Quit |
| R | Restart (on game over) |

## Custom Libraries

| Library | Purpose |
|---------|---------|
| math.c | Multiplication (Russian peasant), division, mod, PRNG |
| string.c | strlen, strcpy, strcmp, int-to-string conversions |
| memory.c | Custom allocator: 64KB virtual RAM, first-fit, coalescing |
| screen.c | ANSI escape rendering: cursor, color, clear |
| keyboard.c | Raw terminal mode, non-blocking input, arrow keys |

## Constraints

- No `<string.h>`, `<math.h>`, or standard `malloc()`/`free()` in game logic
- Only `<stdio.h>` and `<stdlib.h>` from the C standard library
- All values computed dynamically (no hardcoded game logic)

## Known Issues

- Terminal must support 256-color ANSI codes for gradient effects
- `usleep()` is POSIX-obsolescent but works on macOS and Linux
