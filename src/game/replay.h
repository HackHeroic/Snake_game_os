#ifndef REPLAY_H
#define REPLAY_H

#define REPLAY_CAP 2500

typedef struct {
    int x[REPLAY_CAP];
    int y[REPLAY_CAP];
    int len;
} Replay;

void replay_clear(Replay *r);
void replay_push(Replay *r, int x, int y);
void replay_load_best(Replay *r, int *out_best_score);
void replay_try_save_best(const Replay *r, int game_score);
/* Returns 0 if no pos at this tick, else 1 and sets *ox,*oy. */
int replay_step_coords(const Replay *r, int tick, int *ox, int *oy);

#endif
