#ifndef REPLAY_H
#define REPLAY_H

#define REPLAY_MAX_FRAMES 6000

typedef struct {
    short x;
    short y;
} ReplayFrame;

typedef struct {
    ReplayFrame frames[REPLAY_MAX_FRAMES];
    int count;
    int score;            /* score this run achieved */
    int board_w;          /* board the run was recorded on */
    int board_h;
} Replay;

/* Begin a fresh recording for a new game. */
void replay_begin_record(Replay *rec, int board_w, int board_h);

/* Append the snake head position for this tick (no-op once full). */
void replay_record_frame(Replay *rec, int x, int y);

/* Persist this recording if its score beats the saved best. */
void replay_save_if_best(const Replay *rec, int final_score);

/* Load saved best run (returns 1 on success and frames>0). */
int replay_load_best(Replay *out);

#endif
