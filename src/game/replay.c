#include "replay.h"
#include <stdio.h>
#include <stddef.h>

#define REPLAY_FILE "data/best_replay.dat"

void replay_clear(Replay *r) {
    if (r) r->len = 0;
}

void replay_push(Replay *r, int x, int y) {
    if (!r) return;
    if (r->len < REPLAY_CAP) {
        r->x[r->len] = x;
        r->y[r->len] = y;
        r->len++;
    } else {
        r->x[REPLAY_CAP - 1] = x;
        r->y[REPLAY_CAP - 1] = y;
    }
}

void replay_load_best(Replay *r, int *out_best_score) {
    FILE *f;
    int v, n, i;
    if (out_best_score) *out_best_score = 0;
    replay_clear(r);
    f = fopen(REPLAY_FILE, "r");
    if (!f) return;
    if (fscanf(f, "%d %d", &v, &n) != 2) {
        fclose(f);
        return;
    }
    if (out_best_score) *out_best_score = v;
    if (n < 0 || n > REPLAY_CAP) n = 0;
    for (i = 0; i < n; i++) {
        if (fscanf(f, "%d %d", &r->x[i], &r->y[i]) != 2) {
            n = i;
            break;
        }
    }
    r->len = n;
    fclose(f);
}

void replay_try_save_best(const Replay *r, int game_score) {
    FILE *f;
    int best, i, old_len;

    f = fopen(REPLAY_FILE, "r");
    best = -1;
    old_len = 0;
    if (f) {
        if (fscanf(f, "%d %d", &best, &old_len) < 1) {
            best = -1;
        }
        fclose(f);
    }
    if (r->len < 1) return;
    if (game_score > best) {
        f = fopen(REPLAY_FILE, "w");
        if (f) {
            fprintf(f, "%d %d", game_score, r->len);
            for (i = 0; i < r->len; i++) {
                fprintf(f, " %d %d", r->x[i], r->y[i]);
            }
            fprintf(f, "\n");
            fclose(f);
        }
    }
}

int replay_step_coords(const Replay *r, int tick, int *ox, int *oy) {
    if (!r || !ox || !oy) return 0;
    if (tick < 0 || tick >= r->len) return 0;
    *ox = r->x[tick];
    *oy = r->y[tick];
    return 1;
}
