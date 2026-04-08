#ifndef SCREENS_H
#define SCREENS_H

#include "../game/snake.h"
#include "../game/board.h"
#include "../game/score.h"

/* returns game mode: 0=Classic, 1=Wrap. Also accumulates PRNG seed. */
int show_title_screen(int *seed);

void show_pause_overlay(Board *b);
void show_game_over(Snake *s, Board *b, Score *sc);
void show_victory(Score *s, Board *b);

/* returns 'r' for restart or 'q' for quit */
int show_game_over_prompt(Board *b, Score *s);

#endif
