# Snake Game Feature Bundle — Design Spec

**Date:** 2026-04-09
**Scope:** 4 new features layered onto the existing snake game

## Overview

Add gameplay variety and polish to the snake game through four self-contained features:
special food types, obstacles, color themes, and persistent stats tracking. Each feature
builds on the existing architecture with minimal changes to core modules.

---

## Feature 1: Special Food Types

### Description

Two new food types alongside the existing normal food, adding risk/reward decisions.

### Food Types

| Type | Character | Color | Points | Behavior |
|------|-----------|-------|--------|----------|
| Normal | `*` | Theme-dependent (default: red 196) | 1 | Stays until eaten |
| Bonus | `$` | Theme-dependent (default: yellow 226) | 3 | Disappears after 30 ticks |
| Slow | `~` | Theme-dependent (default: blue 39) | 1 | Disappears after 30 ticks; reduces speed by 50ms for 30 ticks |

### Spawn Probability

When food is consumed and new food spawns:
- 65% chance: Normal food
- 20% chance: Bonus food
- 15% chance: Slow food

Use the existing PRNG: generate a random number, take `mod 100`, and check the range.

### Data Changes

Extend `food.h`:
```c
typedef enum { FOOD_NORMAL, FOOD_BONUS, FOOD_SLOW } FoodType;
```

Add fields to the food struct:
- `FoodType type` — which kind of food
- `int ticks_remaining` — countdown for bonus/slow food (0 = no expiry, i.e., normal food)

### Slow Effect

When slow food is eaten:
- Store a `slow_ticks_remaining` counter (initialized to 30) in the game state (a local variable in `main.c`'s game loop)
- Each tick, if `slow_ticks_remaining > 0`, add 50ms to the current frame delay and decrement the counter
- The slow effect does not stack — eating another slow food resets the counter to 30

### Despawn Logic

In the main game loop, each tick:
1. If `food->ticks_remaining > 0`, decrement it
2. If it reaches 0, despawn the food (dealloc + spawn new food)

### Affected Files

- `src/game/food.c/h` — add enum, fields, spawn probability logic
- `src/main.c` — tick countdown, slow effect timer, despawn logic
- `src/game/score.c` — accept variable point value in score update
- `src/ui/renderer.c` — render character/color based on food type

---

## Feature 2: Obstacles

### Description

Static wall segments that appear as the player progresses through levels, increasing difficulty.

### Spawn Rules

- No obstacles at level 0
- On each level-up (every 5 food eaten), spawn 1-2 new obstacles at random positions
- Maximum 10 obstacles on the board at any time
- Obstacles persist for the rest of the game (they don't despawn)

### Placement Safety

An obstacle must NOT spawn:
- On any snake segment
- On the current food position
- Within 2 cells (Manhattan distance) of the snake's head

If a random position violates these rules, retry with a new random position (same pattern as food spawning).

### Collision

- Hitting an obstacle kills the snake (same as wall collision in classic mode)
- This applies in both Classic and Wrap modes
- The collision check runs alongside the existing wall/self checks in the game loop

### Data Structure

Fixed-size array in a new module:
```c
typedef struct {
    int x;
    int y;
} Obstacle;

typedef struct {
    Obstacle items[10];
    int count;
} Obstacles;
```

No dynamic allocation — the max is small and known.

### Rendering

- Character: `#`
- Color: White/gray (250)
- Drawn once when spawned (like the border), not redrawn every frame

### Affected Files

- `src/game/obstacles.c/h` — new file: init, spawn, collision check
- `src/main.c` — call obstacle spawn on level-up, add collision check
- `src/ui/renderer.c` — render obstacle when spawned

---

## Feature 3: Color Themes

### Description

4 selectable color themes chosen on the title screen, replacing hardcoded color values.

### Themes

| Theme | Head | Body Gradient | Food | Border | Obstacle |
|-------|------|---------------|------|--------|----------|
| Classic | 46 (bright green) | 46 -> 40 -> 34 -> 28 -> 22 | 196 (red) | 6 (cyan) | 250 (gray) |
| Ice | 45 (light blue) | 45 -> 39 -> 33 -> 27 -> 21 | 226 (yellow) | 75 (light cyan) | 255 (white) |
| Lava | 202 (orange) | 202 -> 196 -> 160 -> 124 -> 88 | 46 (green) | 208 (dark orange) | 250 (gray) |
| Rainbow | Cycles | segment_index % 6 into [196, 208, 226, 46, 21, 93] | 255 (white) | 6 (cyan) | 250 (gray) |

### Data Structure

```c
typedef struct {
    int head_color;
    int body_colors[5];
    int food_color;
    int bonus_food_color;
    int slow_food_color;
    int obstacle_color;
    int border_color;
    int rainbow;          /* 1 = use rainbow body logic, 0 = use gradient */
} Theme;
```

The 4 themes are static const structs defined in `themes.c`. No allocation.

### Rainbow Logic

For rainbow theme, the body color for a segment is:
```c
int rainbow_colors[] = {196, 208, 226, 46, 21, 93};
color = rainbow_colors[segment_index % 6];
```

This replaces the gradient lookup only when `theme->rainbow == 1`.

### Selection

On the title screen, after choosing game mode (1=Classic, 2=Wrap), a second prompt appears:
```
Theme: 1=Classic  2=Ice  3=Lava  4=Rainbow
```

The selected theme is passed to the renderer at game start.

### Affected Files

- `src/ui/themes.c/h` — new file: theme structs, selection
- `src/ui/renderer.c` — replace hardcoded colors with theme lookups
- `src/ui/screens.c` — add theme selection to title screen

---

## Feature 4: Stats Tracking

### Description

Persistent lifetime statistics tracked across game sessions.

### Stats Tracked

| Stat | Description | Updated When |
|------|-------------|-------------|
| `games_played` | Total games completed | Game over or quit |
| `total_food` | Cumulative food eaten | Each food eaten |
| `longest_snake` | Best snake length ever | Each food eaten (if new record) |
| `best_level` | Highest level reached | Level-up (if new record) |

### Data Structure

```c
typedef struct {
    int games_played;
    int total_food;
    int longest_snake;
    int best_level;
} Stats;
```

### Persistence

- File: `data/stats.dat`
- Format: plain text, one value per line (same pattern as `highscore.dat`)
- `stats_load()`: read via `fscanf`, default all to 0 if file missing
- `stats_save()`: write via `fprintf`

### Display

On the game-over screen, below the existing score/high-score display, show:
```
--- Lifetime Stats ---
Games: 42    Food: 318
Longest: 27  Best Lvl: 5
```

Rendered using `screen_put_str()` — 2-3 extra lines on the game-over screen.

### Flow

1. `stats_load()` at game initialization (once)
2. `stats_update()` called on food eaten (updates `total_food`, `longest_snake`) and on level-up (updates `best_level`)
3. `stats.games_played++` on game over
4. `stats_save()` on game over and on quit

### Affected Files

- `src/game/stats.c/h` — new file: struct, load, save, update
- `src/main.c` — call stats functions at appropriate points
- `src/ui/screens.c` — render stats on game-over screen

---

## Architecture Summary

### New Files

| File | Purpose |
|------|---------|
| `src/game/obstacles.c/h` | Obstacle array, spawning, collision |
| `src/game/stats.c/h` | Lifetime stats, file persistence |
| `src/ui/themes.c/h` | Theme structs, selection |

### Modified Files

| File | Changes |
|------|---------|
| `src/game/food.c/h` | Food type enum, ticks_remaining, spawn probability |
| `src/game/score.c/h` | Accept variable point value |
| `src/ui/renderer.c` | Theme-based colors, food type rendering, obstacle rendering |
| `src/ui/screens.c` | Theme selection on title screen, stats on game-over screen |
| `src/main.c` | Food countdown, slow effect, obstacle spawn/collision, stats calls |
| `Makefile` | Add new .c files to build |

### No Changes To

- `src/lib/math.c/h` — existing PRNG and math sufficient
- `src/lib/string.c/h` — no new string needs
- `src/lib/memory.c/h` — obstacles use static array, themes are const, stats are stack/global
- `src/lib/screen.c/h` — existing screen functions sufficient
- `src/lib/keyboard.c/h` — existing input handling sufficient

### Dependencies Between Features

- **Themes must exist before renderer changes** — renderer needs theme struct to reference
- **Food types and obstacles are independent** of each other
- **Stats is independent** of all other features
- **Renderer changes combine** theme colors + food type rendering + obstacle rendering

Recommended implementation order:
1. Themes (unblocks renderer changes)
2. Special food types
3. Obstacles
4. Stats tracking
