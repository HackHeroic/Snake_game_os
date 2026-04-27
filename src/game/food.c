#include "food.h"
#include "obstacles.h"
#include "score.h"  /* full struct for foods_maintain */
#include "../lib/memory.h"
#include "../lib/math.h"
#include <stddef.h>

void foods_init(Foods *fs) {
    if (fs) fs->count = 0;
}

int foods_target_for_level(int level) {
    int t = 2 + my_divide(level, 3);
    if (t > MAX_FOOD_ON_BOARD) t = MAX_FOOD_ON_BOARD;
    return t;
}

static void food_roll_type(Food *f, int *seed) {
    int roll = my_mod(my_abs(pseudo_random(seed)), 100);
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
}

int foods_is_cell_occupied(Foods *fs, int x, int y) {
    int i;
    for (i = 0; i < fs->count; i++) {
        if (fs->slot[i].x == x && fs->slot[i].y == y) return 1;
    }
    return 0;
}

int foods_try_add(Foods *fs, Snake *s, Obstacles *obs, int board_w, int board_h, int *seed) {
    int x, y;
    int valid;
    SnakeSegment *seg;
    int area;
    int tries;
    int i;

    if (!fs || fs->count >= MAX_FOOD_ON_BOARD) return 0;

    area = my_multiply(board_w, board_h);
    if (s->length >= area - 1) {
        return 0;
    }

    tries = 0;
    while (tries < 800) {
        x = my_mod(my_abs(pseudo_random(seed)), board_w);
        y = my_mod(my_abs(pseudo_random(seed)), board_h);

        valid = 1;
        if (foods_is_cell_occupied(fs, x, y)) valid = 0;

        seg = s->head;
        while (valid && seg != NULL) {
            if (seg->x == x && seg->y == y) {
                valid = 0;
                break;
            }
            seg = seg->next;
        }

        if (valid && obs) {
            for (i = 0; i < obs->count; i++) {
                if (obs->items[i].x == x && obs->items[i].y == y) {
                    valid = 0;
                    break;
                }
            }
        }

        if (valid) {
            food_roll_type(&fs->slot[fs->count], seed);
            fs->slot[fs->count].x = x;
            fs->slot[fs->count].y = y;
            fs->count++;
            return 1;
        }
        tries++;
    }
    return 0;
}

void foods_maintain(Foods *fs, Score *sc, Snake *s, Obstacles *obs, int board_w, int board_h, int *seed) {
    int target;
    if (!fs || !sc) return;
    target = foods_target_for_level(sc->level);
    while (fs->count < target) {
        if (!foods_try_add(fs, s, obs, board_w, board_h, seed)) {
            break;
        }
    }
}

int foods_find_at(Foods *fs, int x, int y) {
    int i;
    for (i = 0; i < fs->count; i++) {
        if (fs->slot[i].x == x && fs->slot[i].y == y) return i;
    }
    return -1;
}

void foods_remove_at(Foods *fs, int idx) {
    int j;
    if (!fs || idx < 0 || idx >= fs->count) return;
    for (j = idx; j < fs->count - 1; j++) {
        fs->slot[j] = fs->slot[j + 1];
    }
    fs->count--;
}

void foods_tick_despawn(Foods *fs, Snake *s, Obstacles *obs, int board_w, int board_h, int *seed, Score *sc) {
    int i;

    if (!fs) return;
    i = 0;
    while (i < fs->count) {
        if (fs->slot[i].ticks_remaining > 0) {
            fs->slot[i].ticks_remaining--;
            if (fs->slot[i].ticks_remaining == 0) {
                foods_remove_at(fs, i);
                foods_maintain(fs, sc, s, obs, board_w, board_h, seed);
                continue;
            }
        }
        i++;
    }
}

void foods_cull_outside(Foods *fs, int board_w, int board_h) {
    int i;
    if (!fs) return;
    i = 0;
    while (i < fs->count) {
        if (fs->slot[i].x < 0 || fs->slot[i].x >= board_w
            || fs->slot[i].y < 0 || fs->slot[i].y >= board_h) {
            foods_remove_at(fs, i);
            continue;
        }
        i++;
    }
}
