#ifndef GHOST_H
#define GHOST_H

#include "snake.h"
#include "board.h"
#include "food.h"
#include "obstacles.h"

Snake *ghost_create(Snake *player, Foods *foods, Obstacles *obs, int bw, int bh, int *seed);
/* Random direction + move; returns 1 if moved (erase old tail). */
int ghost_step(Snake *ghost, Board *board, int *seed);

#endif
