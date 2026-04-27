#ifndef BOARD_H
#define BOARD_H

/*
 * Minimum terminal dimensions so the playfield is valid:
 * board_w = cols - 4 >= 4 (three-segment snake along x), board_h = rows - 6 >= 1.
 */
#define MIN_TERMINAL_COLS 8
#define MIN_TERMINAL_ROWS 7

typedef struct {
    int width, height;
    int mode;  /* 0 = Classic, 1 = Wrap */
    int inset; /* shrink amount: playable area is [inset, w-1-inset] x [inset, h-1-inset] */
} Board;

Board *board_create(int width, int height, int mode);
int board_check_wall_collision(Board *b, int x, int y);
void board_wrap_position(Board *b, int *x, int *y);
void board_free(Board *b);

/* Compute desired inset for the given level (every 3 levels, capped). */
int board_inset_for_level(int board_w, int board_h, int level);

/* Set inset (clamped so playable area stays >= 6x4). */
void board_set_inset(Board *b, int inset);

/* True if (x,y) lies in the playable area. */
int board_in_play_area(const Board *b, int x, int y);
int get_terminal_width(void);
int get_terminal_height(void);

void terminal_install_winch_handler(void);
int terminal_consume_winch(void);

void board_set_size(Board *b, int width, int height);

/* After SIGWINCH, ioctl can lag; brief delay before TIOCGWINSZ. */
void terminal_sync_winsize(void);

/* Set board->width/height from current terminal (no-op if too small). */
void board_sync_from_terminal(Board *b);

/* True when terminal size matches board layout (same as main's tw-4 / th-6). */
int board_matches_terminal(const Board *b);

#endif
