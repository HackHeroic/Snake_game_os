#ifndef OBSTACLES_H
#define OBSTACLES_H

#include "snake.h"
#include "food.h"

#define MAX_OBSTACLES 10

typedef struct {
    int x;
    int y;
} Obstacle;

typedef struct {
    Obstacle items[MAX_OBSTACLES];
    int count;
} Obstacles;

void obstacles_init(Obstacles *obs);
void obstacles_spawn(Obstacles *obs, Snake *s, Food *f, int board_w, int board_h, int *seed);
int obstacles_check_collision(Obstacles *obs, int x, int y);

#endif
