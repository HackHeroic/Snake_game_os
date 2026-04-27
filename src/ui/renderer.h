#ifndef RENDERER_H
#define RENDERER_H

#include "../game/snake.h"
#include "../game/food.h"
#include "../game/board.h"
#include "../game/score.h"
#include "../ui/themes.h"
#include "../game/obstacles.h"
#include "../game/speed_zones.h"

void render_border(Board *b, const Theme *t);
/* Full clear + playfield. Pass snake/ghost NULL; sz NULL skips zones. show_repl + in-bounds = replay dot. */
void render_full_frame(const Theme *t, Board *b, Snake *s, Snake *ghost, Foods *foods,
                      Obstacles *o, Score *sc, const SpeedZones *sz,
                      int rep_x, int rep_y, int show_repl);
void render_foods(Foods *fs, Board *b, const Theme *t);
/* Ghost drawn under the player; second layer uses ghost_color. */
void render_ghost(Snake *g, const Theme *t);
void render_speed_zones(const SpeedZones *sz, Board *b, const Theme *t);
/* Restore a single cell: zone mark or space (no snake/food draw). */
void render_restore_cell(int x, int y, const SpeedZones *sz, Board *b, const Theme *t);
void render_replay_dot(int x, int y, const Theme *t, int on);
void render_snake(Snake *s, const Theme *t);
void render_food(Food *f, Board *b, const Theme *t);
void render_hud(Score *s, Board *b);
void render_erase_tail(int x, int y, Board *b);
void render_obstacles(Obstacles *obs, Board *b, const Theme *t);

#endif
