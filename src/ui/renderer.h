#ifndef RENDERER_H
#define RENDERER_H

#include "../game/snake.h"
#include "../game/food.h"
#include "../game/board.h"
#include "../game/score.h"
#include "../ui/themes.h"
#include "../game/obstacles.h"

void render_border(Board *b, const Theme *t);
void render_snake(Snake *s, const Theme *t);
void render_food(Food *f, Board *b, const Theme *t);
void render_hud(Score *s, Board *b);
void render_erase_tail(int x, int y, Board *b);
void render_obstacles(Obstacles *obs, Board *b, const Theme *t);

#endif
