#include "obstacles.h"
#include "../lib/math.h"

void obstacles_init(Obstacles *obs) {
    obs->count = 0;
}

/* check if position is safe for a new obstacle */
static int is_safe(Obstacles *obs, Snake *s, Foods *foods,
                   int x, int y) {
    SnakeSegment *seg;
    int i;
    int dx, dy, dist;

    /* check existing obstacles */
    for (i = 0; i < obs->count; i++) {
        if (obs->items[i].x == x && obs->items[i].y == y)
            return 0;
    }

    if (foods) {
        for (i = 0; i < foods->count; i++) {
            if (foods->slot[i].x == x && foods->slot[i].y == y) return 0;
        }
    }

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

void obstacles_spawn(Obstacles *obs, Snake *s, Foods *foods,
                     int board_w, int board_h, int *seed) {
    int to_spawn;
    int x, y;
    int retries;

    if (obs->count >= MAX_OBSTACLES)
        return;

    /* spawn 1-2 obstacles */
    to_spawn = 1 + my_mod(my_abs(pseudo_random(seed)), 2);

    while (to_spawn > 0 && obs->count < MAX_OBSTACLES) {
        retries = 100;
        while (retries > 0) {
            x = my_mod(my_abs(pseudo_random(seed)), board_w);
            y = my_mod(my_abs(pseudo_random(seed)), board_h);

            if (is_safe(obs, s, foods, x, y)) {
                obs->items[obs->count].x = x;
                obs->items[obs->count].y = y;
                obs->count++;
                break;
            }
            retries--;
        }
        to_spawn--;
    }
}

void obstacles_cull_outside(Obstacles *obs, int board_w, int board_h) {
    int i, w;

    w = 0;
    for (i = 0; i < obs->count; i++) {
        if (obs->items[i].x >= 0 && obs->items[i].x < board_w
            && obs->items[i].y >= 0 && obs->items[i].y < board_h) {
            if (i != w) {
                obs->items[w] = obs->items[i];
            }
            w++;
        }
    }
    obs->count = w;
}

int obstacles_check_collision(Obstacles *obs, int x, int y) {
    int i;
    for (i = 0; i < obs->count; i++) {
        if (obs->items[i].x == x && obs->items[i].y == y)
            return 1;
    }
    return 0;
}
