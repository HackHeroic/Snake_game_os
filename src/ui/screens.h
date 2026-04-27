#ifndef SCREENS_H
#define SCREENS_H

#include "../game/snake.h"
#include "../game/board.h"
#include "../game/score.h"
#include "../game/stats.h"
#include "../ui/themes.h"
#include "../game/food.h"
#include "../game/obstacles.h"

/* returns game mode: 0=Classic, 1=Wrap. Also accumulates PRNG seed. */
int show_title_screen(int *seed);

/* returns theme index: 0=Classic, 1=Ice, 2=Lava, 3=Rainbow */
int show_theme_screen(void);

void show_pause_overlay(Board *b);
/* Returns 1 if user quit while waiting for minimum terminal size. */
int show_game_over(const Theme *t, Snake *s, Board *b, Foods *foods, Obstacles *o, Score *sc);
void show_victory(Score *s, Board *b);

/* returns 'r' for restart or 'q' for quit. Displays stats on the game over screen. */
int show_game_over_prompt(const Theme *t, Board *b, Score *s, Stats *st, Foods *foods, Obstacles *o);

#endif
