#include "board.h"
#include "../lib/memory.h"
#include "../lib/math.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

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

void terminal_sync_winsize(void) {
    usleep(2000);
}

void board_sync_from_terminal(Board *b) {
    int tw;
    int th;

    if (!b) return;
    tw = get_terminal_width();
    th = get_terminal_height();
    if (tw < MIN_TERMINAL_COLS || th < MIN_TERMINAL_ROWS) return;
    board_set_size(b, tw - 4, th - 6);
}

int board_matches_terminal(const Board *b) {
    int tw;
    int th;

    if (!b) return 1;
    tw = get_terminal_width();
    th = get_terminal_height();
    if (tw < MIN_TERMINAL_COLS || th < MIN_TERMINAL_ROWS) return 0;
    return (tw - 4 == b->width) && (th - 6 == b->height);
}

static volatile sig_atomic_t g_winch_pending;

static void winch_handler(int sig) {
    (void)sig;
    g_winch_pending = 1;
}

void terminal_install_winch_handler(void) {
    struct sigaction sa;
    g_winch_pending = 0;
    sa.sa_handler = winch_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGWINCH, &sa, NULL);
}

int terminal_consume_winch(void) {
    if (g_winch_pending) {
        g_winch_pending = 0;
        return 1;
    }
    return 0;
}

Board *board_create(int width, int height, int mode) {
    Board *b = (Board *)alloc(sizeof(Board));
    if (!b) return NULL;
    b->width = width;
    b->height = height;
    b->mode = mode;
    b->inset = 0;
    return b;
}

int board_check_wall_collision(Board *b, int x, int y) {
    int lo_x = b->inset;
    int lo_y = b->inset;
    int hi_x = b->width - b->inset - 1;
    int hi_y = b->height - b->inset - 1;
    if (x < lo_x || x > hi_x || y < lo_y || y > hi_y) return 1;
    return 0;
}

void board_wrap_position(Board *b, int *x, int *y) {
    int lo_x = b->inset;
    int lo_y = b->inset;
    int play_w = b->width - my_multiply(b->inset, 2);
    int play_h = b->height - my_multiply(b->inset, 2);
    if (play_w < 1) play_w = 1;
    if (play_h < 1) play_h = 1;
    *x = lo_x + my_mod(*x - lo_x + play_w, play_w);
    *y = lo_y + my_mod(*y - lo_y + play_h, play_h);
}

void board_set_size(Board *b, int width, int height) {
    int max_inset;
    if (!b) return;
    b->width = width;
    b->height = height;
    /* Re-clamp inset against the new size */
    max_inset = my_divide(my_min(width, my_multiply(height, 2)) - 4, 2);
    if (max_inset < 0) max_inset = 0;
    if (b->inset > max_inset) b->inset = max_inset;
    if (b->inset < 0) b->inset = 0;
}

int board_inset_for_level(int board_w, int board_h, int level) {
    int target = my_divide(level, 3);
    int max_inset;
    if (target < 0) target = 0;
    /* keep playable area at least 6 wide and 4 tall */
    max_inset = my_divide(board_w - 6, 2);
    if (my_divide(board_h - 4, 2) < max_inset) max_inset = my_divide(board_h - 4, 2);
    if (max_inset < 0) max_inset = 0;
    if (target > max_inset) target = max_inset;
    return target;
}

void board_set_inset(Board *b, int inset) {
    int max_inset;
    if (!b) return;
    if (inset < 0) inset = 0;
    max_inset = my_divide(b->width - 6, 2);
    if (my_divide(b->height - 4, 2) < max_inset) max_inset = my_divide(b->height - 4, 2);
    if (max_inset < 0) max_inset = 0;
    if (inset > max_inset) inset = max_inset;
    b->inset = inset;
}

int board_in_play_area(const Board *b, int x, int y) {
    int lo, hi_x, hi_y;
    if (!b) return 0;
    lo = b->inset;
    hi_x = b->width - b->inset - 1;
    hi_y = b->height - b->inset - 1;
    return (x >= lo && x <= hi_x && y >= lo && y <= hi_y);
}

void board_free(Board *b) {
    if (b) dealloc(b);
}
