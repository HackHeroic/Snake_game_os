#ifndef RENDERER_H
#define RENDERER_H

#include "../game/snake.h"
#include "../game/food.h"
#include "../game/board.h"
#include "../game/score.h"

void render_border(Board *b);
void render_snake(Snake *s);
void render_food(Food *f, Board *b);
void render_hud(Score *s, Board *b);
void render_erase_tail(int x, int y, Board *b);

#endif
