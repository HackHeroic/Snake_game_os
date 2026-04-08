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
