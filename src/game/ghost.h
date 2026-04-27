#ifndef GHOST_H
#define GHOST_H

#include "snake.h"
#include "board.h"
#include "obstacles.h"
#include "food.h"

/* Plan one move for the AI enemy snake. Does not move the player. */
void ghost_step(Snake *ghost, Board *b, Obstacles *o, Foods *f, Snake *player, int *seed);

#endif
