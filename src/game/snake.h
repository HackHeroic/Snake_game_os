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
/* Trim tail if needed so all segments are inside the board. Head out of bounds = death. */
void snake_fit_board(Snake *s, int w, int h);
/* True if (x,y) is any segment (including head). */
int snake_occupies(Snake *s, int x, int y);
void snake_free(Snake *s);

#endif
