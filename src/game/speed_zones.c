#include "speed_zones.h"
#include "../lib/math.h"
#include <stddef.h>

void speed_zones_cull(SpeedZones *sz, int board_w, int board_h) {
    int i, w;
    w = 0;
    for (i = 0; i < sz->count; i++) {
        if (sz->items[i].x >= 0 && sz->items[i].x < board_w
            && sz->items[i].y >= 0 && sz->items[i].y < board_h) {
            if (i != w) {
                sz->items[w] = sz->items[i];
            }
            w++;
        }
    }
    sz->count = w;
}

static int cell_busy(SpeedZones *sz, int placed, int x, int y,
                     Snake *s, Foods *f, Obstacles *o) {
    int i;
    SnakeSegment *seg;
    for (i = 0; i < placed; i++) {
        if (sz->items[i].x == x && sz->items[i].y == y) return 1;
    }
    for (i = 0; f && i < f->count; i++) {
        if (f->slot[i].x == x && f->slot[i].y == y) return 1;
    }
    for (i = 0; o && i < o->count; i++) {
        if (o->items[i].x == x && o->items[i].y == y) return 1;
    }
    seg = s->head;
    while (seg) {
        if (seg->x == x && seg->y == y) return 1;
        seg = seg->next;
    }
    return 0;
}

void speed_zones_init(SpeedZones *sz, int board_w, int board_h, Snake *s, Foods *f, Obstacles *o, int *seed) {
    int n, want, i, x, y, d;

    if (!sz) return;
    sz->count = 0;
    want = 5 + my_mod(my_abs(pseudo_random(seed)), 3);
    if (want > MAX_SPEED_ZONES) want = MAX_SPEED_ZONES;
    n = 0;
    for (i = 0; i < want * 300 && n < want; i++) {
        x = my_mod(my_abs(pseudo_random(seed)), board_w);
        y = my_mod(my_abs(pseudo_random(seed)), board_h);
        if (cell_busy(sz, n, x, y, s, f, o)) continue;
        d = (my_mod(n, 2) == 0) ? -35 : 45;
        sz->items[n].x = x;
        sz->items[n].y = y;
        sz->items[n].delta_ms = d;
        n++;
    }
    sz->count = n;
}

int speed_zones_delta_at(const SpeedZones *sz, int x, int y) {
    int j;
    if (!sz) return 0;
    for (j = 0; j < sz->count; j++) {
        if (sz->items[j].x == x && sz->items[j].y == y) return sz->items[j].delta_ms;
    }
    return 0;
}

int speed_zones_delta_for_step(const SpeedZones *sz, int from_x, int from_y, int to_x, int to_y) {
    int d_to;
    if (!sz) return 0;
    /* Entering: to-cell has the zone. Leaving: to is plain but from was on ':' — one more modulated wait. */
    d_to = speed_zones_delta_at(sz, to_x, to_y);
    if (d_to != 0) return d_to;
    return speed_zones_delta_at(sz, from_x, from_y);
}
