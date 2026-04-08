#include "stats.h"
#include <stdio.h>

void stats_init(Stats *st) {
    st->games_played = 0;
    st->total_food = 0;
    st->longest_snake = 0;
    st->best_level = 0;
}

void stats_load(Stats *st) {
    FILE *f = fopen("data/stats.dat", "r");
    if (f) {
        if (fscanf(f, "%d %d %d %d",
                   &st->games_played, &st->total_food,
                   &st->longest_snake, &st->best_level) != 4) {
            stats_init(st);
        }
        fclose(f);
    } else {
        stats_init(st);
    }
}

void stats_save(Stats *st) {
    FILE *f = fopen("data/stats.dat", "w");
    if (f) {
        fprintf(f, "%d %d %d %d\n",
                st->games_played, st->total_food,
                st->longest_snake, st->best_level);
        fclose(f);
    }
}

void stats_on_food(Stats *st, int snake_length) {
    st->total_food++;
    if (snake_length > st->longest_snake)
        st->longest_snake = snake_length;
}

void stats_on_level(Stats *st, int level) {
    if (level > st->best_level)
        st->best_level = level;
}

void stats_on_game_over(Stats *st) {
    st->games_played++;
}
