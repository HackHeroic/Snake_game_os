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
