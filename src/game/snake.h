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

/* Move snake rigidly so it fits 0..new_w-1, 0..new_h-1. Returns 0 if impossible. */
int snake_refit_to_board(Snake *s, int new_w, int new_h);

/*
 * One shared translation for player + ghost so independent centering cannot
 * stack both snakes on the same cells after resize. ghost may be NULL.
 */
int snakes_refit_union(Snake *player, Snake *ghost, int new_w, int new_h);

/* Refit into the inset-bounded play region [inset, w-inset-1] x [inset, h-inset-1]. */
int snakes_refit_inset(Snake *player, Snake *ghost, int new_w, int new_h, int inset);

int snake_occupies_cell(const Snake *s, int x, int y);
int snake_snakes_overlap(const Snake *a, const Snake *b);

#endif
