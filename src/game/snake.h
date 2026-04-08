#ifndef SNAKE_H
#define SNAKE_H

#include "../lib/math.h"

typedef struct SnakeSegment {
    int x, y;
    struct SnakeSegment *next;  /* toward tail */
} SnakeSegment;

typedef struct {
    SnakeSegment *head;
    SnakeSegment *tail;
    Direction direction;
    int length;
    int alive;
} Snake;

Snake *snake_create(int start_x, int start_y);
void snake_compute_next_head(Snake *s, int *nx, int *ny);
void snake_move(Snake *s, int nx, int ny, int grew);
int snake_check_self_collision(Snake *s, int nx, int ny);
void snake_set_direction(Snake *s, Direction dir);
void snake_free(Snake *s);

#endif
