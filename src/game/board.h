#ifndef BOARD_H
#define BOARD_H

/* Minimum terminal size in columns/rows (for board + border + HUD). */
#define BOARD_MIN_TERM_W 22
#define BOARD_MIN_TERM_H 14

typedef struct {
    int width, height;
    int mode;  /* 0 = Classic, 1 = Wrap */
} Board;

Board *board_create(int width, int height, int mode);
void board_set_size(Board *b, int width, int height);
/* Reads winsize; updates board_w, board_h outputs and b dimensions if b is set. */
int board_sync_to_terminal(Board *b, int *board_w, int *board_h);
int board_check_wall_collision(Board *b, int x, int y);
void board_wrap_position(Board *b, int *x, int *y);
void board_free(Board *b);
int get_terminal_width(void);
int get_terminal_height(void);

#endif
