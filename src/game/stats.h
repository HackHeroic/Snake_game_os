#ifndef STATS_H
#define STATS_H

typedef struct {
    int games_played;
    int total_food;
    int longest_snake;
    int best_level;
} Stats;

void stats_init(Stats *st);
void stats_load(Stats *st);
void stats_save(Stats *st);
void stats_on_food(Stats *st, int snake_length);
void stats_on_level(Stats *st, int level);
void stats_on_game_over(Stats *st);

#endif
