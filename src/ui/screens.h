#ifndef SCREENS_H
#define SCREENS_H

#include "../game/snake.h"
#include "../game/board.h"
#include "../game/score.h"
#include "../game/stats.h"

/* returns game mode: 0=Classic, 1=Wrap. Also accumulates PRNG seed. */
int show_title_screen(int *seed);

/* returns theme index: 0=Classic, 1=Ice, 2=Lava, 3=Rainbow */
int show_theme_screen(void);

void show_pause_overlay(Board *b);
void show_game_over(Snake *s, Board *b, Score *sc);
void show_victory(Score *s, Board *b);

/* returns 'r' for restart or 'q' for quit. Displays stats on the game over screen. */
int show_game_over_prompt(Board *b, Score *s, Stats *st);

#endif
