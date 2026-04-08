# Implementation Details

This document describes the architecture and implementation of the Snake Game OS project.

## Project Structure

```
src/
  lib/           System-level libraries (no game knowledge)
    math.c/h       Arithmetic, PRNG (O(log n) algorithms)
    string.c/h     String manipulation (no <string.h>)
    memory.c/h     Custom allocator (64KB virtual RAM, first-fit)
    screen.c/h     ANSI 256-color terminal rendering
    keyboard.c/h   Raw terminal mode, non-blocking input
  game/          Pure game logic (no rendering dependencies)
    snake.c/h      Snake as linked list, movement, collision
    food.c/h       Food spawning with type selection and collision avoidance
    board.c/h      Board dimensions, wall/wrap collision, terminal size
    score.c/h      Score tracking, leveling, speed, high score persistence
    obstacles.c/h  Obstacle array, safe spawning, collision detection
    stats.c/h      Lifetime statistics, file persistence
  ui/            Rendering and presentation
    renderer.c/h   Frame rendering (border, snake, food, obstacles, HUD)
    screens.c/h    Title, pause, game over, victory screens
    themes.c/h     Color theme definitions and selection
  main.c         Game loop orchestration and lifecycle
tests/           Unit tests for core libraries
data/            Runtime data (high scores, stats)
```

## Architecture

The codebase follows strict separation of concerns:

- **lib/** contains system-level utilities with zero game knowledge. These implement fundamental operations (math, strings, memory, I/O) from scratch.
- **game/** contains pure game logic with no rendering dependencies. Game modules communicate state but never draw to screen.
- **ui/** handles all rendering and presentation, pulling state from game modules.
- **main.c** orchestrates everything: initialization, game loop, lifecycle management.

## Custom Libraries

### math.c - O(log n) Arithmetic

All arithmetic is implemented without `<math.h>`:

- **Multiplication**: Russian peasant algorithm (shift-and-add), O(log n)
- **Division**: Binary long division (shift-and-subtract), O(log n)
- **Modulo**: `a - multiply(divide(a, b), b)`
- **PRNG**: MINSTD Linear Congruential Generator using Schrage's method to avoid overflow
  - Formula: `next = (16807 * seed) % 2147483647`
  - Constants: a=16807, m=2^31-1 (Mersenne prime)

### string.c - No <string.h>

Manual implementations of strlen, strcpy, strcmp, strcat, and integer-to-string conversions.

### memory.c - Custom Allocator

A single `malloc()` call at startup allocates 64KB of virtual RAM. All subsequent allocations use a custom first-fit allocator:

- **Block header** (8 bytes, aligned): `[size: 4B | in_use: 1B | padding: 3B | user_data...]`
- **Block splitting**: Only splits if remainder > 16 bytes (header + minimum 8B data)
- **Forward coalescing**: Merges consecutive free blocks on deallocation
- No external memory allocation in game logic

### screen.c - ANSI Escape Rendering

Uses ANSI escape sequences for 256-color terminal rendering:
- Cursor positioning via `\033[y;xH`
- 256-color foreground via `\033[38;5;Nm`
- Cursor hide/show for flicker-free rendering

### keyboard.c - Raw Terminal Mode

Configures the terminal for real-time input:
- Disables line buffering, echo, and signals via `termios`
- Non-blocking reads (`VMIN=0, VTIME=0`)
- Handles 3-byte arrow key escape sequences (`\033[A/B/C/D`)
- Registers SIGINT handler for clean terminal restoration

## Game Components

### Snake (Linked List)

The snake is a singly-linked list with head and tail pointers:
- **Move**: Allocate new head segment, optionally remove tail (if not growing)
- **Self-collision**: Check candidate head position against all body segments (excluding tail, which vacates same tick)
- **Direction**: Rejects opposite direction changes to prevent instant death

### Food System

Three food types with probability-based spawning:
- **Normal** (`*`, 65% chance): 1 point, stays until eaten
- **Bonus** (`$`, 20% chance): 3 points, disappears after 30 ticks
- **Slow** (`~`, 15% chance): 1 point, temporarily reduces speed by 50ms for 30 ticks, disappears after 30 ticks

Food spawning uses the PRNG to find a valid position (not on snake segments or obstacles) and rolls a second random number for type selection.

### Obstacles

Static wall segments stored in a fixed-size array (max 10):
- Spawn 1-2 obstacles on each level-up (starting from level 1)
- **Placement safety**: Must not overlap snake, food, existing obstacles, or be within 2 cells (Manhattan distance) of the snake's head
- Retry limit of 100 attempts prevents infinite loops on crowded boards
- Collision with an obstacle kills the snake (both Classic and Wrap modes)

### Scoring and Leveling

- **Level** = `food_eaten / 5` (based on food count, not score total, to prevent bonus food from accelerating difficulty)
- **Speed** = `max(80, 200 - level * 15)` milliseconds per tick
- **Slow effect**: Adds 50ms to the level-adjusted delay for 30 ticks (bypasses the 80ms floor)
- **High score**: Persisted to `data/highscore.dat`

### Lifetime Stats

Tracked across all game sessions and persisted to `data/stats.dat`:
- Games played, total food eaten, longest snake length, best level reached
- Displayed on the game-over screen

## Color Themes

Four selectable themes, chosen on the title screen:

| Theme | Snake Head | Body Style | Description |
|-------|-----------|------------|-------------|
| Classic | Bright green | Green gradient (5 shades) | Default theme |
| Ice | Light blue | Blue gradient (5 shades) | Cool tones |
| Lava | Orange | Red-orange gradient (5 shades) | Warm tones |
| Rainbow | Red | Cycles through 6 colors | Full spectrum |

Themes are implemented as static const structs containing color codes for every game element (head, body, food types, obstacles, borders). The renderer reads theme values instead of hardcoded colors.

## Game Loop

Each tick of the game loop:

1. **Input handling** — read key, handle quit/pause/direction
2. **Position computation** — calculate candidate head position
3. **Wall check** — collision (Classic) or wrap (Wrap mode)
4. **Self-collision** — check against all body segments
5. **Obstacle collision** — check against obstacle array
6. **Food check** — detect if head will land on food
7. **Execute move** — add head, remove tail (unless growing)
8. **Post-move updates** — score, stats, level-up, obstacle spawn, food respawn
9. **Food despawn** — countdown and respawn for timed food
10. **Render** — draw snake, update HUD if score changed
11. **Frame delay** — speed-based `usleep()` with optional slow effect

## Rendering Strategy

- **Delta rendering**: Only changed elements are redrawn each frame (tail erase, snake redraw, HUD on score change)
- **Border**: Drawn once at game start (and on unpause)
- **Obstacles**: Drawn once when spawned (and on unpause)
- **Food**: Drawn on spawn/respawn
- **Full redraw**: Only on unpause (after `screen_clear()`)

## Testing

Unit tests cover the core libraries:
- **math**: 31 tests (multiply, divide, mod, abs, min, max, clamp, PRNG)
- **string**: 18 tests (strlen, strcpy, strcmp, strcat, int/str conversions)
- **memory**: 13 tests (alloc, dealloc, coalescing, fragmentation, limits)

Run with `make test`.
