#include "replay.h"
#include <stdio.h>
#include <stddef.h>

#define REPLAY_PATH "data/replay.dat"
#define REPLAY_MAGIC 0x52504C59  /* "RPLY" */

void replay_begin_record(Replay *rec, int board_w, int board_h) {
    if (!rec) return;
    rec->count = 0;
    rec->score = 0;
    rec->board_w = board_w;
    rec->board_h = board_h;
}

void replay_record_frame(Replay *rec, int x, int y) {
    if (!rec) return;
    if (rec->count >= REPLAY_MAX_FRAMES) return;
    rec->frames[rec->count].x = (short)x;
    rec->frames[rec->count].y = (short)y;
    rec->count++;
}

static int load_score_only(int *out_score) {
    FILE *f;
    int magic;
    int score;
    int board_w, board_h;
    int count;

    *out_score = 0;
    f = fopen(REPLAY_PATH, "rb");
    if (!f) return 0;

    if (fread(&magic, sizeof(int), 1, f) != 1) { fclose(f); return 0; }
    if (magic != REPLAY_MAGIC) { fclose(f); return 0; }
    if (fread(&score, sizeof(int), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&board_w, sizeof(int), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&board_h, sizeof(int), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&count, sizeof(int), 1, f) != 1) { fclose(f); return 0; }
    fclose(f);

    *out_score = score;
    return 1;
}

void replay_save_if_best(const Replay *rec, int final_score) {
    FILE *f;
    int magic = REPLAY_MAGIC;
    int prev_best = 0;

    if (!rec) return;
    if (rec->count <= 0) return;

    load_score_only(&prev_best);
    if (final_score <= prev_best) return;

    f = fopen(REPLAY_PATH, "wb");
    if (!f) return;

    fwrite(&magic, sizeof(int), 1, f);
    fwrite(&final_score, sizeof(int), 1, f);
    fwrite(&rec->board_w, sizeof(int), 1, f);
    fwrite(&rec->board_h, sizeof(int), 1, f);
    fwrite(&rec->count, sizeof(int), 1, f);
    fwrite(rec->frames, sizeof(ReplayFrame), (size_t)rec->count, f);
    fclose(f);
}

int replay_load_best(Replay *out) {
    FILE *f;
    int magic;
    size_t n;

    if (!out) return 0;
    out->count = 0;
    out->score = 0;
    out->board_w = 0;
    out->board_h = 0;

    f = fopen(REPLAY_PATH, "rb");
    if (!f) return 0;

    if (fread(&magic, sizeof(int), 1, f) != 1 || magic != REPLAY_MAGIC) {
        fclose(f); return 0;
    }
    if (fread(&out->score, sizeof(int), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&out->board_w, sizeof(int), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&out->board_h, sizeof(int), 1, f) != 1) { fclose(f); return 0; }
    if (fread(&out->count, sizeof(int), 1, f) != 1) { fclose(f); return 0; }

    if (out->count < 0 || out->count > REPLAY_MAX_FRAMES) {
        out->count = 0;
        fclose(f); return 0;
    }

    n = fread(out->frames, sizeof(ReplayFrame), (size_t)out->count, f);
    fclose(f);
    if ((int)n != out->count) { out->count = 0; return 0; }
    return out->count > 0;
}
