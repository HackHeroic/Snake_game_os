#ifndef SPEED_ZONES_H
#define SPEED_ZONES_H

#include "board.h"
#include "snake.h"
#include "obstacles.h"
#include "food.h"

#define MAX_SPEED_ZONES 10

typedef struct {
    int x, y;
    int delta_ms;  /* added to base tick (negative = faster) */
} SpeedZone;

typedef struct {
    SpeedZone items[MAX_SPEED_ZONES];
    int count;
} SpeedZones;

void speed_zones_init(SpeedZones *sz, int board_w, int board_h, Snake *s, Foods *f, Obstacles *o, int *seed);
int speed_zones_delta_at(const SpeedZones *sz, int x, int y);
/* Tick delay delta for a move from (from_x,from_y) to (to_x,to_y). */
int speed_zones_delta_for_step(const SpeedZones *sz, int from_x, int from_y, int to_x, int to_y);
/* Remove zones outside 0..w-1, 0..h-1 */
void speed_zones_cull(SpeedZones *sz, int board_w, int board_h);

#endif
