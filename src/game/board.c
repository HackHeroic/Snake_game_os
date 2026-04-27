#include "board.h"
#include "../lib/memory.h"
#include "../lib/math.h"
#include <sys/ioctl.h>
#include <unistd.h>

int get_terminal_width(void) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) return 80;
    return w.ws_col;
}

int get_terminal_height(void) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) return 24;
    return w.ws_row;
}

Board *board_create(int width, int height, int mode) {
    Board *b = (Board *)alloc(sizeof(Board));
    if (!b) return NULL;
    b->width = width;
    b->height = height;
    b->mode = mode;
    return b;
}

void board_set_size(Board *b, int width, int height) {
    if (!b) return;
    b->width = width;
    b->height = height;
}

int board_sync_to_terminal(Board *b, int *board_w, int *board_h) {
    int tw, th;
    int bw, bh;

    tw = get_terminal_width();
    th = get_terminal_height();
    if (tw < BOARD_MIN_TERM_W || th < BOARD_MIN_TERM_H) {
        return 0;
    }
    bw = tw - 4;
    bh = th - 6;
    if (board_w) *board_w = bw;
    if (board_h) *board_h = bh;
    if (b) {
        b->width = bw;
        b->height = bh;
    }
    return 1;
}

int board_check_wall_collision(Board *b, int x, int y) {
    if (x < 0 || x >= b->width || y < 0 || y >= b->height) {
        return 1;
    }
    return 0;
}

void board_wrap_position(Board *b, int *x, int *y) {
    *x = my_mod(*x + b->width, b->width);
    *y = my_mod(*y + b->height, b->height);
}

void board_free(Board *b) {
    if (b) dealloc(b);
}
