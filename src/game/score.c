#include "score.h"
#include "../lib/memory.h"
#include "../lib/math.h"
#include <stdio.h>

Score *score_create(void) {
    Score *s = (Score *)alloc(sizeof(Score));
    if (!s) return NULL;
    s->score = 0;
    s->high_score = 0;
    s->level = 0;
    return s;
}

void score_increment(Score *s, int points) {
    s->score += points;
    s->level = my_divide(s->score, 5);
    if (s->score > s->high_score) {
        s->high_score = s->score;
    }
}

int score_get_level(Score *s) {
    return s->level;
}

int score_get_speed(Score *s) {
    /* base 200ms, -15ms per level, minimum 80ms */
    int delay = 200 - my_multiply(s->level, 15);
    return my_max(80, delay);
}

void score_save_high(Score *s) {
    FILE *f = fopen("data/highscore.dat", "w");
    if (f) {
        fprintf(f, "%d\n", s->high_score);
        fclose(f);
    }
}

void score_load_high(Score *s) {
    FILE *f = fopen("data/highscore.dat", "r");
    if (f) {
        if (fscanf(f, "%d", &s->high_score) != 1) {
            s->high_score = 0;
        }
        fclose(f);
    } else {
        s->high_score = 0;
    }
}

void score_free(Score *s) {
    if (s) dealloc(s);
}
