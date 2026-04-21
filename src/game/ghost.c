#include "ghost.h"
#include "../lib/math.h"
#include <stddef.h>

Snake *ghost_create(Snake *player, Foods *foods, Obstacles *obs, int bw, int bh, int *seed) {
    int attempts;
    int sx, sy;
    Snake *g;
    SnakeSegment *seg;
    int i;
    int ok;

    if (bw < 4 || bh < 1) return NULL;

    for (attempts = 0; attempts < 400; attempts++) {
        sx = 2 + my_mod(my_abs(pseudo_random(seed)), bw - 2);
        sy = my_mod(my_abs(pseudo_random(seed)), bh);

        g = snake_create(sx, sy);
        if (!g) return NULL;

        ok = 1;
        for (seg = g->head; seg != NULL && ok; seg = seg->next) {
            if (snake_occupies_cell(player, seg->x, seg->y)) ok = 0;
            if (foods) {
                for (i = 0; i < foods->count && ok; i++) {
                    if (foods->pieces[i].x == seg->x && foods->pieces[i].y == seg->y)
                        ok = 0;
                }
            }
            if (obs) {
                for (i = 0; i < obs->count && ok; i++) {
                    if (obs->items[i].x == seg->x && obs->items[i].y == seg->y)
                        ok = 0;
                }
            }
        }

        if (ok) return g;

        snake_free(g);
    }

    return NULL;
}

static void ghost_pick_direction(Snake *ghost, int *seed) {
    int d = my_mod(my_abs(pseudo_random(seed)), 4);
    snake_set_direction(ghost, (Direction)d);
}

int ghost_step(Snake *ghost, Board *board, int *seed) {
    int gx, gy;
    int attempt;

    if (!ghost || !board) return 0;

    for (attempt = 0; attempt < 16; attempt++) {
        ghost_pick_direction(ghost, seed);
        snake_compute_next_head(ghost, &gx, &gy);

        if (board->mode == 0) {
            if (board_check_wall_collision(board, gx, gy)) continue;
        } else {
            board_wrap_position(board, &gx, &gy);
        }

        if (snake_check_self_collision(ghost, gx, gy)) continue;

        snake_move(ghost, gx, gy, 0);
        return 1;
    }

    return 0;
}
