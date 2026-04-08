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
    return f;
}

void food_free(Food *f) {
    if (f) dealloc(f);
}
