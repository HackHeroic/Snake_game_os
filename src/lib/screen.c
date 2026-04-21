#include "screen.h"
#include <stdio.h>

void screen_clear(void) {
    printf("\033[2J\033[H");
}

void screen_goto(int x, int y) {
    /* ANSI uses (row, col) = (y, x), 1-indexed */
    printf("\033[%d;%dH", y, x);
}

void screen_clear_line(int y) {
    screen_goto(1, y);
    printf("\033[2K");
}

void screen_put_char(int x, int y, char c) {
    screen_goto(x, y);
    putchar(c);
}

void screen_put_str(int x, int y, const char *s) {
    screen_goto(x, y);
    while (*s) {
        putchar(*s);
        s++;
    }
}

void screen_set_color(int color_code) {
    printf("\033[38;5;%dm", color_code);
}

void screen_reset_color(void) {
    printf("\033[0m");
}

void screen_hide_cursor(void) {
    printf("\033[?25l");
}

void screen_show_cursor(void) {
    printf("\033[?25h");
}

void screen_flush(void) {
    fflush(stdout);
}
