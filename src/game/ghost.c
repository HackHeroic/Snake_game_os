#include "ghost.h"
#include "../lib/math.h"
#include <stddef.h>

void ghost_step(Snake *ghost, Board *b, Obstacles *o, Foods *f, Snake *player, int *seed) {
    int tx, ty;
    int i;
    int ddx, ddy, dist, bestd;
    int d;
    int have_best;
    Direction best_dir;
    int nx, ny;
    int best_score;
    SnakeSegment *seg;

    (void)seed;
    if (!ghost || !b || !ghost->head || !player || !player->head) return;

    if (f && f->count > 0) {
        bestd = 9999;
        tx = f->slot[0].x;
        ty = f->slot[0].y;
        for (i = 0; i < f->count; i++) {
            ddx = my_abs(ghost->head->x - f->slot[i].x);
            ddy = my_abs(ghost->head->y - f->slot[i].y);
            dist = ddx + ddy;
            if (dist < bestd) {
                bestd = dist;
                tx = f->slot[i].x;
                ty = f->slot[i].y;
            }
        }
    } else {
        tx = player->head->x;
        ty = player->head->y;
    }

    have_best = 0;
    best_score = 9999;
    best_dir = ghost->direction;

    for (d = 0; d < 4; d++) {
        Direction try_dir;
        int rev;

        try_dir = (Direction)d;
        rev = my_abs((int)try_dir - (int)ghost->direction);
        if (rev == 2) continue;

        nx = ghost->head->x;
        ny = ghost->head->y;
        if (try_dir == DIR_UP) ny--;
        else if (try_dir == DIR_DOWN) ny++;
        else if (try_dir == DIR_LEFT) nx--;
        else if (try_dir == DIR_RIGHT) nx++;

        if (b->mode == 0) {
            if (board_check_wall_collision(b, nx, ny)) continue;
        } else {
            board_wrap_position(b, &nx, &ny);
        }
        if (obstacles_check_collision(o, nx, ny)) continue;
        if (snake_check_self_collision(ghost, nx, ny)) continue;
        {
            int blocked2;
            blocked2 = 0;
            for (seg = player->head; seg; seg = seg->next) {
                if (nx == seg->x && ny == seg->y && seg != player->head) {
                    blocked2 = 1;
                    break;
                }
            }
            if (blocked2) continue;
        }

        dist = my_abs(nx - tx) + my_abs(ny - ty);
        if (!have_best || dist < best_score) {
            have_best = 1;
            best_score = dist;
            best_dir = try_dir;
        }
    }
    if (!have_best) return;

    snake_set_direction(ghost, best_dir);
    snake_compute_next_head(ghost, &nx, &ny);
    if (b->mode == 0) {
        if (board_check_wall_collision(b, nx, ny)) return;
    } else {
        board_wrap_position(b, &nx, &ny);
    }
    if (obstacles_check_collision(o, nx, ny)) return;
    if (snake_check_self_collision(ghost, nx, ny)) return;
    for (seg = player->head; seg; seg = seg->next) {
        if (nx == seg->x && ny == seg->y && seg != player->head) return;
    }
    snake_move(ghost, nx, ny, 0);
}
