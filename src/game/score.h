#ifndef SCORE_H
#define SCORE_H

typedef struct {
    int score;
    int high_score;
    int level;
    int food_eaten;
} Score;

Score *score_create(void);
void score_increment(Score *s, int points);
int score_get_level(Score *s);
int score_get_speed(Score *s);
void score_save_high(Score *s);
void score_load_high(Score *s);
void score_free(Score *s);

#endif
