#include "food.h"
#include "obstacles.h"
#include "../lib/math.h"
#include <stddef.h>

void foods_init(Foods *fs) {
    if (fs) fs->count = 0;
}

void foods_clear(Foods *fs) {
    if (fs) fs->count = 0;
}

int foods_target_count(int level) {
    /*
     * Multiple foods from the start: floor is MIN_FOOD_PIECES, then grows with level.
     * foods_fill_to_target stops early if the board has no free cells.
     */
    int t = 1 + level;
    if (t < MIN_FOOD_PIECES) t = MIN_FOOD_PIECES;
    if (t > MAX_FOOD_PIECES) t = MAX_FOOD_PIECES;
    return t;
}

int foods_occupy(const Foods *fs, int x, int y) {
    int i;

    if (!fs) return 0;
    for (i = 0; i < fs->count; i++) {
        if (fs->pieces[i].x == x && fs->pieces[i].y == y) return 1;
    }
    return 0;
}

int foods_find_at(const Foods *fs, int x, int y) {
    int i;

    if (!fs) return -1;
    for (i = 0; i < fs->count; i++) {
        if (fs->pieces[i].x == x && fs->pieces[i].y == y) return i;
    }
    return -1;
}

void foods_remove_at(Foods *fs, int index) {
    if (!fs || index < 0 || index >= fs->count) return;
    fs->pieces[index] = fs->pieces[fs->count - 1];
    fs->count--;
}

static void assign_food_type(Food *f, int *seed) {
    int roll;

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
}

int foods_try_add_one(Foods *fs, Snake *s, Obstacles *obs,
                      int board_w, int board_h, int *seed) {
    Food *slot;
    int x, y;
    int valid;
    SnakeSegment *seg;
    int area;
    int i;
    int retries;

    if (!fs || fs->count >= MAX_FOOD_PIECES) return -1;

    area = my_multiply(board_w, board_h);
    if (s->length >= area - 1) return -1;

    for (retries = 0; retries < 400; retries++) {
        x = my_mod(my_abs(pseudo_random(seed)), board_w);
        y = my_mod(my_abs(pseudo_random(seed)), board_h);

        valid = 1;

        if (foods_occupy(fs, x, y)) valid = 0;

        if (valid) {
            seg = s->head;
            while (seg != NULL) {
                if (seg->x == x && seg->y == y) {
                    valid = 0;
                    break;
                }
                seg = seg->next;
            }
        }

        if (valid && obs) {
            for (i = 0; i < obs->count; i++) {
                if (obs->items[i].x == x && obs->items[i].y == y) {
                    valid = 0;
                    break;
                }
            }
        }

        if (!valid) continue;

        slot = &fs->pieces[fs->count];
        slot->x = x;
        slot->y = y;
        assign_food_type(slot, seed);
        fs->count++;
        return 0;
    }

    return -1;
}

void foods_fill_to_target(Foods *fs, Snake *s, Obstacles *obs,
                          int board_w, int board_h, int *seed, int target) {
    int guard;

    if (!fs) return;
    if (target > MAX_FOOD_PIECES) target = MAX_FOOD_PIECES;

    guard = 0;
    while (fs->count < target && guard < 500) {
        if (foods_try_add_one(fs, s, obs, board_w, board_h, seed) != 0) break;
        guard++;
    }
}
