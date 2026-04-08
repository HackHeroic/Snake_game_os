# Feature Bundle Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add special food types, obstacles, color themes, and stats tracking to the snake game.

**Architecture:** Four features layered on top of the existing game. Themes go first because the renderer needs a Theme struct to replace hardcoded colors. Then food types, obstacles, and stats — each self-contained. All new game modules use static data or stack allocation (no custom allocator needed). The `score_increment` function is refactored so leveling is based on food-eaten count rather than score total.

**Tech Stack:** Pure C (C99), custom math/string/memory/screen/keyboard libraries, Makefile build, ANSI 256-color terminal rendering.

**Spec:** `docs/specs/2026-04-09-feature-bundle-design.md`

---

## File Map

| File | Action | Responsibility |
|------|--------|---------------|
| `src/ui/themes.h` | Create | Theme struct definition + 4 static const theme instances |
| `src/ui/themes.c` | Create | Theme getter by index |
| `src/game/obstacles.h` | Create | Obstacles struct, init/spawn/collision API |
| `src/game/obstacles.c` | Create | Obstacle spawning with safety checks, collision detection |
| `src/game/stats.h` | Create | Stats struct, load/save/update API |
| `src/game/stats.c` | Create | Stats file I/O, update logic |
| `src/game/food.h` | Modify | Add FoodType enum, type and ticks_remaining fields |
| `src/game/food.c` | Modify | Probability-based food type selection in food_spawn |
| `src/game/score.h` | Modify | Add food_eaten field to Score struct |
| `src/game/score.c` | Modify | Level based on food_eaten, not score |
| `src/ui/renderer.h` | Modify | Update signatures to accept Theme pointer |
| `src/ui/renderer.c` | Modify | Use theme colors, render food types, render obstacles |
| `src/ui/screens.h` | Modify | Update show_title_screen signature for theme, add stats param to game over |
| `src/ui/screens.c` | Modify | Theme selection prompt, stats display on game over |
| `src/main.c` | Modify | Wire everything: theme, food countdown, slow effect, obstacles, stats |
| `Makefile` | Modify | Add new .c files to build |

---

### Task 1: Create Theme Module

**Files:**
- Create: `src/ui/themes.h`
- Create: `src/ui/themes.c`

- [ ] **Step 1: Create `src/ui/themes.h`**

```c
#ifndef THEMES_H
#define THEMES_H

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

/* rainbow body colors: {196, 208, 226, 46, 21, 93} */
#define NUM_RAINBOW_COLORS 6

const Theme *theme_get(int index);
int theme_get_rainbow_color(int segment_index);

#endif
```

- [ ] **Step 2: Create `src/ui/themes.c`**

```c
#include "themes.h"

static const int rainbow_colors[NUM_RAINBOW_COLORS] = {196, 208, 226, 46, 21, 93};

static const Theme themes[4] = {
    /* Classic */
    { 46, {46, 40, 34, 28, 22}, 196, 226, 39, 250, 45, 0 },
    /* Ice */
    { 45, {45, 39, 33, 27, 21}, 226, 214, 195, 255, 75, 0 },
    /* Lava */
    { 202, {202, 196, 160, 124, 88}, 46, 226, 33, 250, 208, 0 },
    /* Rainbow */
    { 196, {196, 208, 226, 46, 21}, 255, 226, 39, 250, 45, 1 }
};

const Theme *theme_get(int index) {
    if (index < 0 || index > 3) index = 0;
    return &themes[index];
}

int theme_get_rainbow_color(int segment_index) {
    int idx = segment_index % NUM_RAINBOW_COLORS;
    if (idx < 0) idx += NUM_RAINBOW_COLORS;
    return rainbow_colors[idx];
}
```

- [ ] **Step 3: Verify it compiles**

Run: `gcc -Wall -Wextra -Werror -Isrc -c -o src/ui/themes.o src/ui/themes.c`
Expected: No errors, produces `src/ui/themes.o`

- [ ] **Step 4: Commit**

```bash
git add src/ui/themes.h src/ui/themes.c
git commit -m "Add theme module with 4 color themes (classic, ice, lava, rainbow)"
```

---

### Task 2: Wire Themes into Renderer

**Files:**
- Modify: `src/ui/renderer.h`
- Modify: `src/ui/renderer.c`

- [ ] **Step 1: Update `src/ui/renderer.h` — add Theme parameter to functions**

Replace the entire header with:

```c
#ifndef RENDERER_H
#define RENDERER_H

#include "../game/snake.h"
#include "../game/food.h"
#include "../game/board.h"
#include "../game/score.h"
#include "../ui/themes.h"

void render_border(Board *b, const Theme *t);
void render_snake(Snake *s, const Theme *t);
void render_food(Food *f, Board *b, const Theme *t);
void render_hud(Score *s, Board *b);
void render_erase_tail(int x, int y, Board *b);

#endif
```

- [ ] **Step 2: Update `render_border` in `src/ui/renderer.c` to use theme**

Replace the existing `render_border` function. Change the color from hardcoded `45` to `t->border_color`:

```c
void render_border(Board *b, const Theme *t) {
    int i;

    screen_set_color(t->border_color);

    /* top border */
    screen_put_char(OFFSET_X - 1, OFFSET_Y - 1, '+');
    for (i = 0; i < b->width; i++) {
        screen_put_char(OFFSET_X + i, OFFSET_Y - 1, '-');
    }
    screen_put_char(OFFSET_X + b->width, OFFSET_Y - 1, '+');

    /* bottom border */
    screen_put_char(OFFSET_X - 1, OFFSET_Y + b->height, '+');
    for (i = 0; i < b->width; i++) {
        screen_put_char(OFFSET_X + i, OFFSET_Y + b->height, '-');
    }
    screen_put_char(OFFSET_X + b->width, OFFSET_Y + b->height, '+');

    /* side borders */
    for (i = 0; i < b->height; i++) {
        screen_put_char(OFFSET_X - 1, OFFSET_Y + i, '|');
        screen_put_char(OFFSET_X + b->width, OFFSET_Y + i, '|');
    }

    screen_reset_color();
}
```

- [ ] **Step 3: Update `render_snake` in `src/ui/renderer.c` to use theme**

Replace the existing `render_snake` function. Remove the static `snake_colors` array at the top of the file and the `NUM_SNAKE_COLORS` define. Use theme colors and rainbow logic:

```c
void render_snake(Snake *s, const Theme *t) {
    SnakeSegment *seg = s->head;
    int dist = 0;
    int color;

    while (seg != NULL) {
        if (dist == 0) {
            screen_set_color(t->head_color);
            screen_put_char(OFFSET_X + seg->x, OFFSET_Y + seg->y, '@');
        } else {
            if (t->rainbow) {
                color = theme_get_rainbow_color(dist);
            } else {
                color = t->body_colors[my_min(dist - 1, 4)];
            }
            screen_set_color(color);
            screen_put_char(OFFSET_X + seg->x, OFFSET_Y + seg->y, 'O');
        }

        dist++;
        seg = seg->next;
    }

    screen_reset_color();
}
```

- [ ] **Step 4: Update `render_food` in `src/ui/renderer.c` to use theme**

Replace the existing `render_food` function. Use food type to pick character and color:

```c
void render_food(Food *f, Board *b, const Theme *t) {
    int color;
    char ch;

    (void)b;

    if (f->type == FOOD_BONUS) {
        color = t->bonus_food_color;
        ch = '$';
    } else if (f->type == FOOD_SLOW) {
        color = t->slow_food_color;
        ch = '~';
    } else {
        color = t->food_color;
        ch = '*';
    }

    screen_set_color(color);
    screen_put_char(OFFSET_X + f->x, OFFSET_Y + f->y, ch);
    screen_reset_color();
}
```

Note: This step depends on the `FoodType` enum existing in `food.h`. We'll add that in Task 3. For now, this won't compile yet — that's expected. We'll verify compilation after Task 3.

- [ ] **Step 5: Add `#include "../ui/themes.h"` to top of `src/ui/renderer.c`**

Add after the existing includes:

```c
#include "themes.h"
```

Also remove these lines from the top of renderer.c (they're replaced by theme):

```c
/* green gradient colors for snake body (256-color codes) */
static const int snake_colors[] = { 46, 40, 34, 28, 22 };
#define NUM_SNAKE_COLORS 5
```

- [ ] **Step 6: Commit**

```bash
git add src/ui/renderer.h src/ui/renderer.c
git commit -m "Wire theme colors into renderer (border, snake, food)"
```

---

### Task 3: Add Food Types to Food Module

**Files:**
- Modify: `src/game/food.h`
- Modify: `src/game/food.c`

- [ ] **Step 1: Update `src/game/food.h` — add FoodType enum and new fields**

Replace the entire header with:

```c
#ifndef FOOD_H
#define FOOD_H

#include "snake.h"

typedef enum { FOOD_NORMAL, FOOD_BONUS, FOOD_SLOW } FoodType;

typedef struct {
    int x, y;
    FoodType type;
    int ticks_remaining;   /* 0 = no expiry (normal), >0 = countdown */
} Food;

Food *food_spawn(Snake *s, int board_w, int board_h, int *seed);
void food_free(Food *f);

#endif
```

- [ ] **Step 2: Update `src/game/food.c` — add probability-based type selection**

Replace the entire file with:

```c
#include "food.h"
#include "../lib/memory.h"
#include "../lib/math.h"
#include <stddef.h>

Food *food_spawn(Snake *s, int board_w, int board_h, int *seed) {
    Food *f;
    int x, y;
    int valid;
    SnakeSegment *seg;
    int area;
    int roll;

    /* victory guard: if snake fills almost entire board, no room for food */
    area = my_multiply(board_w, board_h);
    if (s->length >= area - 1) {
        return NULL;
    }

    f = (Food *)alloc(sizeof(Food));
    if (!f) return NULL;

    do {
        x = my_mod(my_abs(pseudo_random(seed)), board_w);
        y = my_mod(my_abs(pseudo_random(seed)), board_h);

        valid = 1;
        seg = s->head;
        while (seg != NULL) {
            if (seg->x == x && seg->y == y) {
                valid = 0;
                break;
            }
            seg = seg->next;
        }
    } while (!valid);

    f->x = x;
    f->y = y;

    /* determine food type by probability: 65% normal, 20% bonus, 15% slow */
    roll = my_mod(my_abs(pseudo_random(seed)), 100);
    if (roll < 65) {
        f->type = FOOD_NORMAL;
        f->ticks_remaining = 0;
    } else if (roll < 85) {
        f->type = FOOD_BONUS;
        f->ticks_remaining = 30;
    } else {
        f->type = FOOD_SLOW;
        f->ticks_remaining = 30;
    }

    return f;
}

void food_free(Food *f) {
    if (f) dealloc(f);
}
```

- [ ] **Step 3: Verify themes + food + renderer compile together**

Run: `gcc -Wall -Wextra -Werror -Isrc -c -o src/game/food.o src/game/food.c && gcc -Wall -Wextra -Werror -Isrc -c -o src/ui/renderer.o src/ui/renderer.c`
Expected: No errors

- [ ] **Step 4: Commit**

```bash
git add src/game/food.h src/game/food.c
git commit -m "Add food types (normal, bonus, slow) with probability-based spawning"
```

---

### Task 4: Update Score to Use food_eaten for Leveling

**Files:**
- Modify: `src/game/score.h`
- Modify: `src/game/score.c`

- [ ] **Step 1: Update `src/game/score.h` — add food_eaten field**

Replace the Score struct and add a new function:

```c
#ifndef SCORE_H
#define SCORE_H

typedef struct {
    int score;
    int high_score;
    int level;
    int food_eaten;
} Score;

Score *score_create(void);
void score_increment(Score *s, int points);
int score_get_level(Score *s);
int score_get_speed(Score *s);
void score_save_high(Score *s);
void score_load_high(Score *s);
void score_free(Score *s);

#endif
```

- [ ] **Step 2: Update `src/game/score.c` — level from food_eaten**

Replace `score_create` and `score_increment`:

```c
Score *score_create(void) {
    Score *s = (Score *)alloc(sizeof(Score));
    if (!s) return NULL;
    s->score = 0;
    s->high_score = 0;
    s->level = 0;
    s->food_eaten = 0;
    return s;
}

void score_increment(Score *s, int points) {
    s->score += points;
    s->food_eaten++;
    s->level = my_divide(s->food_eaten, 5);
    if (s->score > s->high_score) {
        s->high_score = s->score;
    }
}
```

- [ ] **Step 3: Verify it compiles**

Run: `gcc -Wall -Wextra -Werror -Isrc -c -o src/game/score.o src/game/score.c`
Expected: No errors

- [ ] **Step 4: Commit**

```bash
git add src/game/score.h src/game/score.c
git commit -m "Base leveling on food_eaten count instead of score total"
```

---

### Task 5: Create Obstacles Module

**Files:**
- Create: `src/game/obstacles.h`
- Create: `src/game/obstacles.c`

- [ ] **Step 1: Create `src/game/obstacles.h`**

```c
#ifndef OBSTACLES_H
#define OBSTACLES_H

#include "snake.h"
#include "food.h"

#define MAX_OBSTACLES 10

typedef struct {
    int x;
    int y;
} Obstacle;

typedef struct {
    Obstacle items[MAX_OBSTACLES];
    int count;
} Obstacles;

void obstacles_init(Obstacles *obs);
void obstacles_spawn(Obstacles *obs, Snake *s, Food *f, int board_w, int board_h, int *seed);
int obstacles_check_collision(Obstacles *obs, int x, int y);

#endif
```

- [ ] **Step 2: Create `src/game/obstacles.c`**

```c
#include "obstacles.h"
#include "../lib/math.h"

void obstacles_init(Obstacles *obs) {
    obs->count = 0;
}

/* check if position is safe for a new obstacle */
static int is_safe(Obstacles *obs, Snake *s, Food *f,
                   int x, int y) {
    SnakeSegment *seg;
    int i;
    int dx, dy, dist;

    /* check existing obstacles */
    for (i = 0; i < obs->count; i++) {
        if (obs->items[i].x == x && obs->items[i].y == y)
            return 0;
    }

    /* check food */
    if (f && f->x == x && f->y == y)
        return 0;

    /* check snake segments */
    seg = s->head;
    while (seg) {
        if (seg->x == x && seg->y == y)
            return 0;
        seg = seg->next;
    }

    /* check within 2 cells of snake head (Manhattan distance) */
    dx = my_abs(s->head->x - x);
    dy = my_abs(s->head->y - y);
    dist = dx + dy;
    if (dist <= 2)
        return 0;

    return 1;
}

void obstacles_spawn(Obstacles *obs, Snake *s, Food *f,
                     int board_w, int board_h, int *seed) {
    int to_spawn;
    int x, y;

    if (obs->count >= MAX_OBSTACLES)
        return;

    /* spawn 1-2 obstacles */
    to_spawn = 1 + my_mod(my_abs(pseudo_random(seed)), 2);

    while (to_spawn > 0 && obs->count < MAX_OBSTACLES) {
        x = my_mod(my_abs(pseudo_random(seed)), board_w);
        y = my_mod(my_abs(pseudo_random(seed)), board_h);

        if (is_safe(obs, s, f, x, y)) {
            obs->items[obs->count].x = x;
            obs->items[obs->count].y = y;
            obs->count++;
            to_spawn--;
        }
    }
}

int obstacles_check_collision(Obstacles *obs, int x, int y) {
    int i;
    for (i = 0; i < obs->count; i++) {
        if (obs->items[i].x == x && obs->items[i].y == y)
            return 1;
    }
    return 0;
}
```

- [ ] **Step 3: Verify it compiles**

Run: `gcc -Wall -Wextra -Werror -Isrc -c -o src/game/obstacles.o src/game/obstacles.c`
Expected: No errors

- [ ] **Step 4: Commit**

```bash
git add src/game/obstacles.h src/game/obstacles.c
git commit -m "Add obstacles module with safe spawning and collision detection"
```

---

### Task 6: Add Obstacle Rendering to Renderer

**Files:**
- Modify: `src/ui/renderer.h`
- Modify: `src/ui/renderer.c`

- [ ] **Step 1: Add render_obstacles to `src/ui/renderer.h`**

Add this include and function declaration:

```c
#include "../game/obstacles.h"
```

Add after the existing declarations:

```c
void render_obstacles(Obstacles *obs, Board *b, const Theme *t);
```

- [ ] **Step 2: Add render_obstacles to `src/ui/renderer.c`**

Add at the end of the file, before any closing comments:

```c
void render_obstacles(Obstacles *obs, Board *b, const Theme *t) {
    int i;
    (void)b;

    screen_set_color(t->obstacle_color);
    for (i = 0; i < obs->count; i++) {
        screen_put_char(OFFSET_X + obs->items[i].x,
                       OFFSET_Y + obs->items[i].y, '#');
    }
    screen_reset_color();
}
```

- [ ] **Step 3: Verify it compiles**

Run: `gcc -Wall -Wextra -Werror -Isrc -c -o src/ui/renderer.o src/ui/renderer.c`
Expected: No errors

- [ ] **Step 4: Commit**

```bash
git add src/ui/renderer.h src/ui/renderer.c
git commit -m "Add obstacle rendering with theme-based color"
```

---

### Task 7: Create Stats Module

**Files:**
- Create: `src/game/stats.h`
- Create: `src/game/stats.c`

- [ ] **Step 1: Create `src/game/stats.h`**

```c
#ifndef STATS_H
#define STATS_H

typedef struct {
    int games_played;
    int total_food;
    int longest_snake;
    int best_level;
} Stats;

void stats_init(Stats *st);
void stats_load(Stats *st);
void stats_save(Stats *st);
void stats_on_food(Stats *st, int snake_length);
void stats_on_level(Stats *st, int level);
void stats_on_game_over(Stats *st);

#endif
```

- [ ] **Step 2: Create `src/game/stats.c`**

```c
#include "stats.h"
#include <stdio.h>

void stats_init(Stats *st) {
    st->games_played = 0;
    st->total_food = 0;
    st->longest_snake = 0;
    st->best_level = 0;
}

void stats_load(Stats *st) {
    FILE *f = fopen("data/stats.dat", "r");
    if (f) {
        if (fscanf(f, "%d %d %d %d",
                   &st->games_played, &st->total_food,
                   &st->longest_snake, &st->best_level) != 4) {
            stats_init(st);
        }
        fclose(f);
    } else {
        stats_init(st);
    }
}

void stats_save(Stats *st) {
    FILE *f = fopen("data/stats.dat", "w");
    if (f) {
        fprintf(f, "%d %d %d %d\n",
                st->games_played, st->total_food,
                st->longest_snake, st->best_level);
        fclose(f);
    }
}

void stats_on_food(Stats *st, int snake_length) {
    st->total_food++;
    if (snake_length > st->longest_snake)
        st->longest_snake = snake_length;
}

void stats_on_level(Stats *st, int level) {
    if (level > st->best_level)
        st->best_level = level;
}

void stats_on_game_over(Stats *st) {
    st->games_played++;
}
```

- [ ] **Step 3: Verify it compiles**

Run: `gcc -Wall -Wextra -Werror -Isrc -c -o src/game/stats.o src/game/stats.c`
Expected: No errors

- [ ] **Step 4: Commit**

```bash
git add src/game/stats.h src/game/stats.c
git commit -m "Add stats module with persistent lifetime statistics"
```

---

### Task 8: Update Screens for Theme Selection and Stats Display

**Files:**
- Modify: `src/ui/screens.h`
- Modify: `src/ui/screens.c`

- [ ] **Step 1: Update `src/ui/screens.h` — new signatures**

Replace the entire header with:

```c
#ifndef SCREENS_H
#define SCREENS_H

#include "../game/snake.h"
#include "../game/board.h"
#include "../game/score.h"
#include "../game/stats.h"

/* returns game mode: 0=Classic, 1=Wrap. Also accumulates PRNG seed. */
int show_title_screen(int *seed);

/* returns theme index: 0=Classic, 1=Ice, 2=Lava, 3=Rainbow */
int show_theme_screen(void);

void show_pause_overlay(Board *b);
void show_game_over(Snake *s, Board *b, Score *sc);
void show_victory(Score *s, Board *b);

/* returns 'r' for restart or 'q' for quit. Displays stats on the game over screen. */
int show_game_over_prompt(Board *b, Score *s, Stats *st);

#endif
```

- [ ] **Step 2: Add `show_theme_screen` to `src/ui/screens.c`**

Add after the `show_title_screen` function:

```c
int show_theme_screen(void) {
    int key;

    screen_put_str(5, 16, "Select Theme:");
    screen_set_color(46);
    screen_put_str(5, 18, "[1] Classic    (green)");
    screen_set_color(45);
    screen_put_str(5, 19, "[2] Ice        (blue)");
    screen_set_color(202);
    screen_put_str(5, 20, "[3] Lava       (red/orange)");
    screen_set_color(196);
    screen_put_str(5, 21, "[4] Rainbow    (multicolor)");
    screen_reset_color();

    screen_flush();

    while (1) {
        key = read_key();
        if (key == '1') return 0;
        if (key == '2') return 1;
        if (key == '3') return 2;
        if (key == '4') return 3;
        usleep(10000);
    }
}
```

- [ ] **Step 3: Update `show_game_over_prompt` in `src/ui/screens.c` to display stats**

Replace the entire `show_game_over_prompt` function:

```c
int show_game_over_prompt(Board *b, Score *s, Stats *st) {
    int cx = OFFSET_X + my_divide(b->width, 2) - 4;
    int cy = OFFSET_Y + my_divide(b->height, 2);
    char score_str[16];
    char high_str[16];
    char line[64];
    char num_str[16];
    int key;

    screen_set_color(196);
    screen_put_str(cx, cy - 1, "GAME OVER");
    screen_reset_color();

    int_to_str(s->score, score_str);
    int_to_str(s->high_score, high_str);

    my_strcpy(line, "Score: ");
    my_strcat(line, score_str);
    my_strcat(line, "  High: ");
    my_strcat(line, high_str);
    screen_put_str(cx - 2, cy + 1, line);

    screen_put_str(cx - 4, cy + 3, "R: Restart  |  Q: Quit");

    /* lifetime stats */
    screen_set_color(240);
    screen_put_str(cx - 5, cy + 5, "--- Lifetime Stats ---");

    my_strcpy(line, "Games: ");
    int_to_str(st->games_played, num_str);
    my_strcat(line, num_str);
    my_strcat(line, "    Food: ");
    int_to_str(st->total_food, num_str);
    my_strcat(line, num_str);
    screen_put_str(cx - 5, cy + 6, line);

    my_strcpy(line, "Longest: ");
    int_to_str(st->longest_snake, num_str);
    my_strcat(line, num_str);
    my_strcat(line, "  Best Lvl: ");
    int_to_str(st->best_level, num_str);
    my_strcat(line, num_str);
    screen_put_str(cx - 5, cy + 7, line);
    screen_reset_color();

    screen_flush();

    while (1) {
        key = read_key();
        if (key == 'r' || key == 'R') return 'r';
        if (key == 'q' || key == 'Q') return 'q';
        usleep(16000);
    }
}
```

- [ ] **Step 4: Add `#include "../game/stats.h"` to the includes in `src/ui/screens.c`**

Add after the existing includes at the top of the file:

```c
#include "../game/stats.h"
```

- [ ] **Step 5: Verify it compiles**

Run: `gcc -Wall -Wextra -Werror -Isrc -c -o src/ui/screens.o src/ui/screens.c`
Expected: No errors

- [ ] **Step 6: Commit**

```bash
git add src/ui/screens.h src/ui/screens.c
git commit -m "Add theme selection screen and stats display on game over"
```

---

### Task 9: Update Makefile

**Files:**
- Modify: `Makefile`

- [ ] **Step 1: Add new source files to Makefile**

Replace the `GAME_SRC` and `UI_SRC` lines:

```makefile
GAME_SRC = src/game/snake.c src/game/food.c src/game/board.c src/game/score.c src/game/obstacles.c src/game/stats.c
UI_SRC = src/ui/renderer.c src/ui/screens.c src/ui/themes.c
```

- [ ] **Step 2: Verify clean build**

Run: `make clean && make`
Expected: Compiles with no errors. Binary `snake` is produced.

- [ ] **Step 3: Commit**

```bash
git add Makefile
git commit -m "Add new source files to Makefile (obstacles, stats, themes)"
```

---

### Task 10: Wire Everything into main.c

**Files:**
- Modify: `src/main.c`

- [ ] **Step 1: Update includes and add new variables**

Replace the includes and variable declarations at the top of main:

```c
#include "lib/memory.h"
#include "lib/screen.h"
#include "lib/keyboard.h"
#include "lib/math.h"
#include "game/snake.h"
#include "game/food.h"
#include "game/board.h"
#include "game/score.h"
#include "game/obstacles.h"
#include "game/stats.h"
#include "ui/renderer.h"
#include "ui/screens.h"
#include "ui/themes.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    int seed = 0;
    int mode;
    int theme_idx;
    const Theme *theme;
    int running = 1;
    Board *board;
    Snake *snake;
    Food *food;
    Score *score;
    Obstacles obstacles;
    Stats stats;
    int term_w, term_h;
    int board_w, board_h;
    int paused;
    int key;
    int nx, ny;
    int grew;
    int old_tail_x, old_tail_y;
    int last_score;
    int prev_level;
    int slow_ticks;
    int speed;
    int points;
```

- [ ] **Step 2: Add stats_load and theme selection to game start**

Replace from `/* init systems */` through the title screen and up to the `while (running)` loop opening:

```c
    /* init systems */
    memory_init();
    keyboard_init();
    screen_hide_cursor();

    /* load lifetime stats once */
    stats_load(&stats);

    while (running) {
        /* title screen — also seeds PRNG */
        mode = show_title_screen(&seed);

        /* theme selection */
        theme_idx = show_theme_screen();
        theme = theme_get(theme_idx);
```

- [ ] **Step 3: Replace game object creation and initial draw**

Replace from `/* compute board from terminal size */` through `screen_flush();` (the initial draw section):

```c
        /* compute board from terminal size */
        term_w = get_terminal_width();
        term_h = get_terminal_height();

        if (term_w < 22 || term_h < 14) {
            keyboard_restore();
            screen_show_cursor();
            screen_clear();
            printf("Terminal too small! Need at least 22x14, got %dx%d\n",
                   term_w, term_h);
            return 1;
        }

        /* board is terminal minus borders and HUD space */
        board_w = term_w - 4;   /* 2 for border + 2 margin */
        board_h = term_h - 6;   /* 2 for border + 2 HUD lines + 2 margin */

        /* create game objects */
        board = board_create(board_w, board_h, mode);
        snake = snake_create(my_divide(board_w, 2), my_divide(board_h, 2));
        score = score_create();
        score_load_high(score);
        food = food_spawn(snake, board_w, board_h, &seed);
        obstacles_init(&obstacles);

        /* initial draw */
        screen_clear();
        render_border(board, theme);
        render_snake(snake, theme);
        if (food) render_food(food, board, theme);
        render_hud(score, board);
        screen_flush();

        paused = 0;
        last_score = 0;
        prev_level = 0;
        slow_ticks = 0;
```

- [ ] **Step 4: Replace the unpause redraw block**

Replace the unpause block (the `else` after `show_pause_overlay`) to include obstacles:

```c
                } else {
                    /* redraw to clear pause text */
                    screen_clear();
                    render_border(board, theme);
                    render_snake(snake, theme);
                    if (food) render_food(food, board, theme);
                    render_obstacles(&obstacles, board, theme);
                    render_hud(score, board);
                    screen_flush();
                }
```

- [ ] **Step 5: Replace the food despawn + obstacle collision section**

After the self-collision check and before the food check, add obstacle collision. Also add food despawn logic. Replace from `/* 3. self-collision check */` through `grew = (food && nx == food->x && ny == food->y);`:

```c
            /* 3. self-collision check */
            if (snake_check_self_collision(snake, nx, ny)) {
                snake->alive = 0;
                break;
            }

            /* 4. obstacle collision check */
            if (obstacles_check_collision(&obstacles, nx, ny)) {
                snake->alive = 0;
                break;
            }

            /* 5. food check (before move) */
            grew = (food && nx == food->x && ny == food->y);
```

- [ ] **Step 6: Replace the post-move updates section**

Replace from `/* 6. post-move updates */` through the end of the `if (grew)` block:

```c
            /* 6. post-move updates */
            if (grew) {
                /* determine points from food type */
                points = (food->type == FOOD_BONUS) ? 3 : 1;

                /* apply slow effect if slow food */
                if (food->type == FOOD_SLOW) {
                    slow_ticks = 30;
                }

                score_increment(score, points);
                stats_on_food(&stats, snake->length);

                /* check for level-up and spawn obstacles */
                if (score->level > prev_level) {
                    stats_on_level(&stats, score->level);
                    obstacles_spawn(&obstacles, snake, food,
                                   board_w, board_h, &seed);
                    render_obstacles(&obstacles, board, theme);
                    prev_level = score->level;
                }

                food_free(food);
                food = food_spawn(snake, board_w, board_h, &seed);
                if (food == NULL) {
                    /* victory! board is full */
                    show_victory(score, board);
                    score_save_high(score);
                    stats_on_game_over(&stats);
                    stats_save(&stats);
                    usleep(3000000);
                    break;
                }
                render_food(food, board, theme);
            } else {
                render_erase_tail(old_tail_x, old_tail_y, board);
            }
```

- [ ] **Step 7: Replace the render + wait section**

Replace from `/* 7. render and wait */` through `usleep(...)`:

```c
            /* 7. food despawn countdown */
            if (food && food->ticks_remaining > 0) {
                food->ticks_remaining--;
                if (food->ticks_remaining == 0) {
                    /* erase old food and spawn new */
                    screen_put_char(OFFSET_X + food->x, OFFSET_Y + food->y, ' ');
                    food_free(food);
                    food = food_spawn(snake, board_w, board_h, &seed);
                    if (food) render_food(food, board, theme);
                }
            }

            /* 8. render and wait */
            render_snake(snake, theme);
            if (score->score != last_score) {
                render_hud(score, board);
                last_score = score->score;
            }
            screen_flush();

            /* apply speed with optional slow effect */
            speed = score_get_speed(score);
            if (slow_ticks > 0) {
                speed += 50;
                slow_ticks--;
            }
            usleep(my_multiply(speed, 1000));
```

Note: `OFFSET_X` and `OFFSET_Y` are defined in renderer.c but not exposed. We need to define them here too. Add at the top of main.c after the includes:

```c
#define OFFSET_X 2
#define OFFSET_Y 2
```

- [ ] **Step 8: Replace the game over sequence**

Replace the game over sequence block:

```c
        /* game over sequence (only if not quitting) */
        if (running && !snake->alive) {
            stats_on_game_over(&stats);
            stats_save(&stats);
            show_game_over(snake, board, score);
            score_save_high(score);

            key = show_game_over_prompt(board, score, &stats);
            if (key == 'q') {
                running = 0;
            }
        }
```

- [ ] **Step 9: Full build and manual test**

Run: `make clean && make`
Expected: Clean build with no errors or warnings.

Run: `make run`
Expected: Game starts, shows title screen, then theme selection. Play through:
- Verify food types appear (normal `*`, bonus `$`, slow `~`) with correct colors per theme
- Verify obstacles (`#`) spawn on level-up
- Verify slow food temporarily slows the snake
- Verify bonus food gives 3 points
- Verify timed food disappears after ~30 ticks
- Verify obstacle collision kills the snake
- Verify pause/unpause redraws obstacles
- Verify game over screen shows lifetime stats
- Verify stats persist across restarts (R key)

- [ ] **Step 10: Commit**

```bash
git add src/main.c
git commit -m "Wire all features into game loop (food types, obstacles, themes, stats)"
```

---

### Task 11: Update README

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Update README to document new features**

Add a section after the existing features documentation describing:
- Special food types (bonus `$` and slow `~`) with their behaviors
- Obstacles that spawn on level-up
- 4 color themes selectable on the title screen
- Lifetime stats displayed on game over
- Updated controls info mentioning theme selection

- [ ] **Step 2: Commit**

```bash
git add README.md
git commit -m "Update README with new features documentation"
```

---

## Implementation Order Summary

| Task | What | Dependencies |
|------|------|-------------|
| 1 | Theme module | None |
| 2 | Wire themes into renderer | Task 1 |
| 3 | Food types in food module | None (but renderer in Task 2 references FoodType) |
| 4 | Score uses food_eaten for leveling | None |
| 5 | Obstacles module | None |
| 6 | Obstacle rendering | Task 2, Task 5 |
| 7 | Stats module | None |
| 8 | Screens (theme selection + stats display) | Task 7 |
| 9 | Update Makefile | Tasks 1, 5, 7 |
| 10 | Wire everything in main.c | All above |
| 11 | Update README | Task 10 |
