#ifndef OBSTACLES_H
#define OBSTACLES_H

#include "snake.h"
#include "food.h" /* Foods */

#define MAX_OBSTACLES 10

typedef struct {
    int x;
    int y;
} Obstacle;

typedef struct Obstacles {
    Obstacle items[MAX_OBSTACLES];
    int count;
} Obstacles;

void obstacles_init(Obstacles *obs);
void obstacles_spawn(Obstacles *obs, Snake *s, Foods *foods,
                     int board_w, int board_h, int *seed);
int obstacles_check_collision(Obstacles *obs, int x, int y);
void obstacles_clamp_to_board(Obstacles *obs, int board_w, int board_h);

#endif
