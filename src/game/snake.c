#include "snake.h"
#include "../lib/memory.h"
#include <stddef.h>

/* direction deltas: UP, RIGHT, DOWN, LEFT */
static const int dx[] = { 0, 1, 0, -1 };
static const int dy[] = { -1, 0, 1, 0 };

Snake *snake_create(int start_x, int start_y) {
    Snake *s;
    SnakeSegment *seg;
    int i;

    s = (Snake *)alloc(sizeof(Snake));
    if (!s) return NULL;

    s->head = NULL;
    s->tail = NULL;
    s->direction = DIR_RIGHT;
    s->length = 0;
    s->alive = 1;

    /* create 3 segments: head at start_x, body trailing left */
    for (i = 2; i >= 0; i--) {
        seg = (SnakeSegment *)alloc(sizeof(SnakeSegment));
        if (!seg) return s;

        seg->x = start_x - i;
        seg->y = start_y;
        seg->next = NULL;

        if (s->head == NULL) {
            s->head = seg;
            s->tail = seg;
        } else {
            seg->next = s->head;
            s->head = seg;
        }
        s->length++;
    }

    return s;
}

void snake_compute_next_head(Snake *s, int *nx, int *ny) {
    *nx = s->head->x + dx[s->direction];
    *ny = s->head->y + dy[s->direction];
}

void snake_move(Snake *s, int nx, int ny, int grew) {
    SnakeSegment *new_head;

    new_head = (SnakeSegment *)alloc(sizeof(SnakeSegment));
    if (!new_head) return;

    new_head->x = nx;
    new_head->y = ny;
    new_head->next = s->head;
    s->head = new_head;

    if (!grew) {
        /* remove tail */
        SnakeSegment *prev = s->head;
        while (prev->next != s->tail) {
            prev = prev->next;
        }
        dealloc(s->tail);
        s->tail = prev;
        s->tail->next = NULL;
    } else {
        s->length++;
    }
}

int snake_check_self_collision(Snake *s, int nx, int ny) {
    SnakeSegment *seg = s->head;
    while (seg != NULL) {
        /* Skip tail: in the non-growing case, the tail will be removed
           during this tick, so its cell is effectively free */
        if (seg != s->tail && seg->x == nx && seg->y == ny) {
            return 1;
        }
        seg = seg->next;
    }
    return 0;
}

void snake_set_direction(Snake *s, Direction dir) {
    int diff = my_abs((int)dir - (int)s->direction);
    if (diff == 2) return;
    s->direction = dir;
}

static void expand_bbox(Snake *s, int *min_x, int *max_x, int *min_y, int *max_y) {
    SnakeSegment *seg;

    for (seg = s->head; seg != NULL; seg = seg->next) {
        if (seg->x < *min_x) *min_x = seg->x;
        if (seg->x > *max_x) *max_x = seg->x;
        if (seg->y < *min_y) *min_y = seg->y;
        if (seg->y > *max_y) *max_y = seg->y;
    }
}

int snakes_refit_union(Snake *player, Snake *ghost, int new_w, int new_h) {
    SnakeSegment *seg;
    int min_x, max_x, min_y, max_y;
    int bw, bh;
    int ox, oy;

    if (!player || !player->head) return 0;

    min_x = max_x = player->head->x;
    min_y = max_y = player->head->y;
    expand_bbox(player, &min_x, &max_x, &min_y, &max_y);
    if (ghost && ghost->head) expand_bbox(ghost, &min_x, &max_x, &min_y, &max_y);

    bw = max_x - min_x + 1;
    bh = max_y - min_y + 1;
    if (bw > new_w || bh > new_h) return 0;

    ox = -min_x + my_divide(new_w - bw, 2);
    oy = -min_y + my_divide(new_h - bh, 2);

    for (seg = player->head; seg != NULL; seg = seg->next) {
        seg->x += ox;
        seg->y += oy;
    }
    if (ghost && ghost->head) {
        for (seg = ghost->head; seg != NULL; seg = seg->next) {
            seg->x += ox;
            seg->y += oy;
        }
    }

    for (seg = player->head; seg != NULL; seg = seg->next) {
        if (seg->x < 0 || seg->x >= new_w || seg->y < 0 || seg->y >= new_h)
            return 0;
    }
    if (ghost && ghost->head) {
        for (seg = ghost->head; seg != NULL; seg = seg->next) {
            if (seg->x < 0 || seg->x >= new_w || seg->y < 0 || seg->y >= new_h)
                return 0;
        }
    }
    return 1;
}

int snake_refit_to_board(Snake *s, int new_w, int new_h) {
    return snakes_refit_union(s, NULL, new_w, new_h);
}

int snakes_refit_inset(Snake *player, Snake *ghost, int new_w, int new_h, int inset) {
    SnakeSegment *seg;
    int min_x, max_x, min_y, max_y;
    int bw, bh;
    int play_w, play_h;
    int ox, oy;
    int lo;

    if (!player || !player->head) return 0;
    if (inset < 0) inset = 0;

    play_w = new_w - my_multiply(inset, 2);
    play_h = new_h - my_multiply(inset, 2);
    if (play_w < 1 || play_h < 1) return 0;

    min_x = max_x = player->head->x;
    min_y = max_y = player->head->y;
    expand_bbox(player, &min_x, &max_x, &min_y, &max_y);
    if (ghost && ghost->head) expand_bbox(ghost, &min_x, &max_x, &min_y, &max_y);

    bw = max_x - min_x + 1;
    bh = max_y - min_y + 1;
    if (bw > play_w || bh > play_h) return 0;

    lo = inset;
    ox = -min_x + lo + my_divide(play_w - bw, 2);
    oy = -min_y + lo + my_divide(play_h - bh, 2);

    for (seg = player->head; seg != NULL; seg = seg->next) {
        seg->x += ox;
        seg->y += oy;
    }
    if (ghost && ghost->head) {
        for (seg = ghost->head; seg != NULL; seg = seg->next) {
            seg->x += ox;
            seg->y += oy;
        }
    }

    return 1;
}

int snake_occupies_cell(const Snake *s, int x, int y) {
    SnakeSegment *seg;

    if (!s) return 0;
    for (seg = s->head; seg != NULL; seg = seg->next) {
        if (seg->x == x && seg->y == y) return 1;
    }
    return 0;
}

int snake_snakes_overlap(const Snake *a, const Snake *b) {
    SnakeSegment *sa;
    SnakeSegment *sb;

    if (!a || !b) return 0;
    for (sa = a->head; sa != NULL; sa = sa->next) {
        for (sb = b->head; sb != NULL; sb = sb->next) {
            if (sa->x == sb->x && sa->y == sb->y) return 1;
        }
    }
    return 0;
}

void snake_free(Snake *s) {
    SnakeSegment *seg;
    SnakeSegment *next;

    if (!s) return;

    seg = s->head;
    while (seg != NULL) {
        next = seg->next;
        dealloc(seg);
        seg = next;
    }
    dealloc(s);
}
