#ifndef FOOD_H
#define FOOD_H

#include "snake.h"

typedef enum { FOOD_NORMAL, FOOD_BONUS, FOOD_SLOW } FoodType;

typedef struct {
    int x, y;
    FoodType type;
    int ticks_remaining;   /* 0 = no expiry (normal), >0 = countdown */
} Food;

Food *food_spawn(Snake *s, int board_w, int board_h, int *seed);
void food_free(Food *f);

#endif
