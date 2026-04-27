#ifndef FOOD_H
#define FOOD_H

#include "snake.h"
#include "score.h"

/* forward declaration to avoid circular dependency with obstacles.h */
typedef struct Obstacles Obstacles;

typedef enum { FOOD_NORMAL, FOOD_BONUS, FOOD_SLOW } FoodType;

typedef struct {
    int x, y;
    FoodType type;
    int ticks_remaining;   /* 0 = no expiry (normal), >0 = countdown */
} Food;

#define MAX_FOOD_ON_BOARD 4

typedef struct {
    Food slot[MAX_FOOD_ON_BOARD];
    int count;
} Foods;

void foods_init(Foods *fs);
int foods_target_for_level(int level);
/* Add one item if under cap and a cell exists. Returns 0 on failure. */
int foods_try_add(Foods *fs, Snake *s, Obstacles *obs, int board_w, int board_h, int *seed);
/* Top up to target for current level. */
void foods_maintain(Foods *fs, Score *sc, Snake *s, Obstacles *obs, int board_w, int board_h, int *seed);
int foods_find_at(Foods *fs, int x, int y);
void foods_remove_at(Foods *fs, int idx);
/* Decrement timed food; on expiry remove and try to respawn. */
void foods_tick_despawn(Foods *fs, Snake *s, Obstacles *obs, int board_w, int board_h, int *seed, Score *sc);
int foods_is_cell_occupied(Foods *fs, int x, int y);
/* Remove any food outside [0..w) x [0..h) */
void foods_cull_outside(Foods *fs, int board_w, int board_h);

#endif
