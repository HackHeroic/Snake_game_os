#ifndef FOOD_H
#define FOOD_H

#include "snake.h"

/* forward declaration to avoid circular dependency with obstacles.h */
typedef struct Obstacles Obstacles;

typedef enum { FOOD_NORMAL, FOOD_BONUS, FOOD_SLOW } FoodType;

typedef struct {
    int x, y;
    FoodType type;
    int ticks_remaining;   /* 0 = no expiry (normal), >0 = countdown */
} Food;

#define MAX_FOOD_PIECES 8
/* Always try to keep at least this many on the board (fewer if no empty cells). */
#define MIN_FOOD_PIECES 3

typedef struct {
    Food pieces[MAX_FOOD_PIECES];
    int count;
} Foods;

void foods_init(Foods *fs);
void foods_clear(Foods *fs);

/* Target pieces: max(MIN_FOOD_PIECES, 1 + level), capped at MAX_FOOD_PIECES. */
int foods_target_count(int level);

int foods_occupy(const Foods *fs, int x, int y);
int foods_find_at(const Foods *fs, int x, int y);
void foods_remove_at(Foods *fs, int index);

/* Returns 0 on success, -1 if no valid cell (board full per legacy rule). */
int foods_try_add_one(Foods *fs, Snake *s, Obstacles *obs,
                      int board_w, int board_h, int *seed);

void foods_fill_to_target(Foods *fs, Snake *s, Obstacles *obs,
                          int board_w, int board_h, int *seed, int target);

#endif
