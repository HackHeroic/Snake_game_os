#ifndef POWERUPS_H
#define POWERUPS_H

#include "snake.h"
#include "food.h"
#include "obstacles.h"

typedef enum { POW_BOOST, POW_SLOW } PowerupType;

typedef struct {
    int x, y;
    PowerupType type;
    int ticks_remaining;   /* zone lifetime on the board */
    int glow_phase;        /* used to animate the glow */
} Powerup;

#define MAX_POWERUPS 3
#define POWERUP_LIFETIME 120     /* ticks zone stays before despawn */
#define POWERUP_EFFECT_TICKS 25  /* effect duration after passing through */

typedef struct {
    Powerup zones[MAX_POWERUPS];
    int count;
    int spawn_cooldown;          /* ticks until next spawn attempt */
} Powerups;

void powerups_init(Powerups *p);
void powerups_clear(Powerups *p);

/* Decrement lifetimes, despawn expired zones. Returns 1 if any zone expired. */
int powerups_tick(Powerups *p);

/* Try to spawn one zone if under cap and cooldown elapsed. */
void powerups_maybe_spawn(Powerups *p, Snake *snake, Snake *ghost, Foods *foods,
                          Obstacles *obs, int board_w, int board_h, int inset,
                          int *seed);

/* Returns index of zone at (x,y) or -1. */
int powerups_find_at(const Powerups *p, int x, int y);

void powerups_remove_at(Powerups *p, int index);

#endif
