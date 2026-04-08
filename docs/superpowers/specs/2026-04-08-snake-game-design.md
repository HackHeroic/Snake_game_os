# Snake Game — Design Specification

## 1. Overview

A real-time terminal-based Snake game written entirely in C, for an OS course capstone project (Track A). The project demonstrates low-level systems programming through 5 custom-built libraries: math, string, memory, screen, and keyboard. No standard C libraries are used except `<stdio.h>` (terminal I/O) and `<stdlib.h>` (initial memory setup + process exit).

### Features
- Classic snake gameplay with real-time input
- Two game modes: Classic (wall death) and Wrap-around (snake wraps edges)
- Speed increases as score grows (levels)
- Pause/Resume
- Persistent high score system (file-based)
- Gradient-colored snake, styled borders, ASCII art title screen, game-over animations
- WASD + Arrow key controls

## 2. File Structure

```
snake-game-os/
├── src/
│   ├── lib/
│   │   ├── math.h / math.c
│   │   ├── string.h / string.c
│   │   ├── memory.h / memory.c
│   │   ├── screen.h / screen.c
│   │   └── keyboard.h / keyboard.c
│   ├── game/
│   │   ├── snake.h / snake.c
│   │   ├── food.h / food.c
│   │   ├── board.h / board.c
│   │   └── score.h / score.c
│   ├── ui/
│   │   ├── renderer.h / renderer.c
│   │   └── screens.h / screens.c
│   └── main.c
├── data/
│   └── highscore.dat
├── Makefile
└── README.md
```

**Separation of concerns:**
- `lib/` — custom libraries. Know nothing about the game.
- `game/` — game logic. Uses libraries but knows nothing about rendering.
- `ui/` — rendering and screens. Pulls game state and renders it.
- `main.c` — init, game loop, cleanup.

## 3. Custom Libraries

### 3.1 math.c / math.h

Custom implementations of arithmetic and boundary operations. No `<math.h>`.

**Functions:**
- `int my_multiply(int a, int b)` — integer multiplication via repeated addition
- `int my_divide(int a, int b)` — integer division via repeated subtraction
- `int my_mod(int a, int b)` — modulo via divide and subtract
- `int my_abs(int a)` — absolute value
- `int my_min(int a, int b)` / `int my_max(int a, int b)`
- `int my_clamp(int val, int min, int max)` — clamp value to range
- `int pseudo_random(int *seed)` — LCG-based PRNG: `next = (a * seed + c) % m`

### 3.2 string.c / string.h

Custom string manipulation. No `<string.h>`.

**Functions:**
- `int my_strlen(const char *s)`
- `void my_strcpy(char *dest, const char *src)`
- `int my_strcmp(const char *a, const char *b)`
- `void int_to_str(int n, char *buf)` — integer to string conversion
- `int str_to_int(const char *s)` — string to integer conversion
- `void my_strcat(char *dest, const char *src)` — string concatenation

### 3.3 memory.c / memory.h

A mini-malloc implementation over a virtual RAM region. No `malloc()`/`free()` for game logic.

**Design:**
- A single `char` array (64KB) allocated once via `malloc()` at program startup — the only allowed use of `malloc`.
- First-fit allocator with block headers:
  ```
  [ size: 4 bytes | in_use: 1 byte | ---- user data ---- ]
  ```
- `void *alloc(int size)` — finds first free block large enough, splits if oversized, returns pointer past header.
- `void dealloc(void *ptr)` — marks block as free, coalesces adjacent free blocks.
- `void memory_init()` — initializes the virtual RAM as one large free block.
- `int memory_used()` — returns bytes currently in use (for leak detection/debugging).

### 3.4 screen.c / screen.h

Terminal rendering via ANSI escape codes. Uses `<stdio.h>` `printf` only.

**Functions:**
- `void screen_clear()` — `\033[2J\033[H`
- `void screen_goto(int x, int y)` — `\033[y;xH`
- `void screen_put_char(int x, int y, char c)` — position + print char
- `void screen_put_str(int x, int y, const char *s)` — position + print string
- `void screen_set_color(int color_code)` — `\033[38;5;Nm` (256-color)
- `void screen_reset_color()` — `\033[0m`
- `void screen_hide_cursor()` / `void screen_show_cursor()`

### 3.5 keyboard.c / keyboard.h

Non-blocking keyboard input using POSIX terminal raw mode (`<termios.h>`, `<unistd.h>`).

**Functions:**
- `void keyboard_init()` — switch terminal to raw mode (disable line buffering, echo)
- `void keyboard_restore()` — restore original terminal settings
- `int key_pressed()` — non-blocking read. Returns 0 if no key, or the key code.
- `int read_key()` — reads a full key event (handles 3-byte arrow key escape sequences: `\033[A/B/C/D`)
- `void read_line(char *buf, int max)` — blocking full-line read (for menus if needed)

**Arrow key handling:** Detect `\033` prefix, read next 2 bytes to determine arrow direction. Map to same direction constants as WASD.

## 4. Game Components

### 4.1 Snake (snake.c)

**Data structures:**
```c
typedef struct SnakeSegment {
    int x, y;
    struct SnakeSegment *next;  // toward tail
} SnakeSegment;

typedef struct Snake {
    SnakeSegment *head;
    SnakeSegment *tail;
    int direction;      // UP=0, RIGHT=1, DOWN=2, LEFT=3
    int length;
    int alive;
} Snake;
```

**Operations:**
- `Snake *snake_create(int start_x, int start_y)` — allocates initial snake (3 segments)
- `void snake_move(Snake *s, int grew)` — alloc new head in current direction, dealloc tail unless `grew`
- `int snake_check_self_collision(Snake *s)` — walk list, compare head coords with each body segment
- `void snake_set_direction(Snake *s, int dir)` — reject opposite direction
- `void snake_free(Snake *s)` — walk and dealloc every segment

**Movement logic:**
1. Compute new head `(x, y)` from current head + direction delta (via `math.c`)
2. `alloc()` new `SnakeSegment`
3. Link as new head
4. If not growing: `dealloc()` tail, advance tail pointer
5. If growing: keep tail (length++)

### 4.2 Food (food.c)

- `Food *food_spawn(Snake *s, int board_w, int board_h, int *seed)` — generate random position via `pseudo_random()` + `my_mod()`, verify not on snake body, retry if collision
- Uses `math.c` for all coordinate computation

### 4.3 Board (board.c)

- `Board *board_create(int width, int height)` — stores dimensions
- `int board_check_wall_collision(Board *b, int x, int y)` — returns 1 if out of bounds, using `math.c` clamp/comparisons
- `int board_wrap_position(Board *b, int *x, int *y)` — wraps coordinates for wrap-around mode
- Dimensions computed from terminal size at startup (no hardcoded values)

### 4.4 Score (score.c)

- `void score_increment(Score *s, int points)` — increase score, recalculate level
- `int score_get_level(Score *s)` — `score / 5` (every 5 food = level up), via `my_divide()`
- `int score_get_speed(Score *s)` — tick delay decreases with level, computed via `math.c`
- `void score_save_high(Score *s)` — write to `data/highscore.dat` via `fprintf`
- `void score_load_high(Score *s)` — read from file via `fscanf`, default 0 if missing

**Speed formula (via math.c):**
- Base tick: 200ms
- Per level reduction: 15ms
- Minimum tick: 80ms
- `tick = max(80, 200 - level * 15)`

## 5. UI Components

### 5.1 Renderer (renderer.c)

**Delta rendering:** Only redraw what changed each frame:
- Erase old tail position (draw space)
- Draw new head position (with bright green color)
- Update gradient colors on existing segments (shift down the green palette)
- Redraw HUD only when score/level changes

**Snake gradient colors (256-color mode):**
- Head: color 46 (bright green), character `@`
- Body segments fade: 46 → 40 → 34 → 28 → 22 (darker greens), character `O`
- Color assigned based on distance from head

**Food:** Color 196 (bright red), character `*`

**Borders:** Box-drawing characters (`─`, `│`, `┌`, `┐`, `└`, `┘`) in color 45 (cyan)

**HUD layout (below board):**
```
Score: 15  |  High: 42  |  Level: 3  |  Mode: Classic
Controls: WASD/Arrows  |  P: Pause  |  Q: Quit
```

### 5.2 Screens (screens.c)

**Title screen:**
- ASCII art "SNAKE" logo in green gradient
- Mode selection: `[1] Classic  [2] Wrap-Around`
- Waits for player choice before starting

**Pause overlay:**
- "PAUSED" rendered in center of board
- "Press P to resume"

**Game-over screen:**
- "GAME OVER" text
- Final score and high score
- "Press R to Restart | Q to Quit"

**Victory screen:**
- Triggered when snake fills the entire board
- "YOU WIN!" with final stats

## 6. Game Loop (main.c)

```
main():
    memory_init()
    keyboard_init()
    show title screen → get mode choice
    score_load_high()
    create board, snake, food

    loop:
        key = read_key()             [non-blocking]
        if key == 'p' → toggle pause
        if key == 'q' → break
        if paused → render pause overlay, continue

        snake_set_direction(key)
        snake_move()

        if wall collision:
            if classic mode → game over
            if wrap mode → board_wrap_position()

        if self collision → game over

        if head on food:
            score_increment()
            food_spawn()
            (snake already grew via move logic)

        render frame (delta)
        usleep(tick_duration)

    game over:
        show game-over screen
        score_save_high() if new record
        wait for R (restart) or Q (quit)
        if restart → free everything, loop back
        if quit → cleanup and exit

    cleanup:
        snake_free()
        keyboard_restore()
        screen_show_cursor()
        screen_clear()
```

## 7. Edge Cases

- **Fast input:** Only process the last valid direction change per tick
- **Food on snake:** Regenerate position until valid
- **Terminal too small:** Detect size at startup, refuse to start if below 20x10
- **Snake fills board:** Victory condition
- **Unexpected exit:** Cleanup function restores terminal settings

## 8. Memory Leak Prevention

- Every `alloc()` has a corresponding `dealloc()` path
- On restart: walk entire snake list, free each node, then reinitialize
- On quit: same cleanup
- `memory_used()` can verify zero bytes in use after cleanup

## 9. Allowed Headers

| Header | Usage | Justification |
|--------|-------|---------------|
| `<stdio.h>` | printf, fprintf, fscanf, fopen | Terminal I/O, file I/O (explicitly allowed) |
| `<stdlib.h>` | Initial malloc for virtual RAM, exit() | Process start/exit (explicitly allowed) |
| `<termios.h>` | Raw terminal mode | Hardware abstraction (terminal is hardware) |
| `<unistd.h>` | read(), usleep() | Hardware abstraction (system calls) |

## 10. Build System

**Makefile** with:
- `make` — build the game
- `make clean` — remove build artifacts
- `make run` — build and run
- Compiler: `gcc` with `-Wall -Wextra -Werror`
- All `.c` files compiled to `.o`, linked into single `snake` binary

## 11. Constraints Compliance

- No `<string.h>` — all string ops in custom `string.c`
- No `<math.h>` — all math in custom `math.c`
- No `malloc()`/`free()` for game logic — all dynamic allocation through custom `memory.c`
- No hardcoded game values — dimensions from terminal, speed from formula, positions from PRNG
