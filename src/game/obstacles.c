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
