#include "keyboard.h"
#include "screen.h"
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

static struct termios original_termios;
static int raw_mode_enabled = 0;

/* Set by SIGWINCH; read with keyboard_consume_resize in the game loop. */
static volatile sig_atomic_t terminal_resized = 0;

static void sigwinch_handler(int sig) {
    (void)sig;
    terminal_resized = 1;
}

static void sigint_handler(int sig) {
    (void)sig;
    keyboard_restore();
    screen_show_cursor();
    screen_reset_color();
    printf("\n");
    _exit(0);
}

void keyboard_init(void) {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) return;

    raw = original_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return;
    raw_mode_enabled = 1;

    signal(SIGINT, sigint_handler);
    signal(SIGWINCH, sigwinch_handler);
}

void keyboard_restore(void) {
    if (raw_mode_enabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
        raw_mode_enabled = 0;
    }
}

int key_pressed(void) {
    unsigned char c;
    int n = read(STDIN_FILENO, &c, 1);
    if (n <= 0) return KEY_NONE;
    return (int)c;
}

int read_key(void) {
    unsigned char c;
    int n = read(STDIN_FILENO, &c, 1);
    if (n <= 0) return KEY_NONE;

    if (c == '\033') {
        unsigned char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\033';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\033';

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
            }
        }
        return '\033';
    }

    return (int)c;
}

int keyboard_consume_resize(void) {
    if (terminal_resized) {
        terminal_resized = 0;
        return 1;
    }
    return 0;
}
