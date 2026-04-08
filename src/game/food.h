#ifndef FOOD_H
#define FOOD_H

#include "snake.h"

typedef struct {
    int x, y;
} Food;

Food *food_spawn(Snake *s, int board_w, int board_h, int *seed);
void food_free(Food *f);

#endif
