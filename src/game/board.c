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
    return b;
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

void board_set_size(Board *b, int width, int height) {
    if (!b) return;
    b->width = width;
    b->height = height;
}

void board_free(Board *b) {
    if (b) dealloc(b);
}
