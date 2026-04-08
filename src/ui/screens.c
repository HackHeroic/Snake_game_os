#include "screens.h"
#include "../lib/screen.h"
#include "../lib/keyboard.h"
#include "../lib/string.h"
#include "../lib/math.h"
#include "../game/stats.h"
#include <unistd.h>
#include <stddef.h>

#define OFFSET_X 2
#define OFFSET_Y 2

static const char *snake_logo[] = {
    " ____  _   _    _    _  _______",
    "/ ___|| \\ | |  / \\  | |/ / ____|",
    "\\___ \\|  \\| | / _ \\ | ' /|  _|",
    " ___) | |\\  |/ ___ \\| . \\| |___",
    "|____/|_| \\_/_/   \\_\\_|\\_\\_____|",
    NULL
};

static int logo_color(int line) {
    int colors[] = { 46, 40, 34, 28, 22 };
    if (line < 0) line = 0;
    if (line > 4) line = 4;
    return colors[line];
}

int show_title_screen(int *seed) {
    int key;
    int i;
    int entropy = 0;
    int tick = 0;

    screen_clear();

    /* draw logo with gradient */
    for (i = 0; snake_logo[i] != NULL; i++) {
        screen_set_color(logo_color(i));
        screen_put_str(5, 3 + i, snake_logo[i]);
    }
    screen_reset_color();

    screen_put_str(5, 10, "Select Game Mode:");
    screen_set_color(46);
    screen_put_str(5, 12, "[1] Classic  (walls kill)");
    screen_put_str(5, 13, "[2] Wrap-Around  (walls wrap)");
    screen_reset_color();

    screen_flush();

    /* wait for valid input, accumulate entropy for PRNG seed */
    while (1) {
        key = read_key();
        tick++;
        entropy += tick;
        if (key == '1') { *seed = entropy; return 0; }
        if (key == '2') { *seed = entropy; return 1; }
        usleep(10000);  /* 10ms poll interval */
    }
}

int show_theme_screen(void) {
    int key;

    screen_put_str(5, 16, "Select Theme:");
    screen_set_color(46);
    screen_put_str(5, 18, "[1] Classic    (green)");
    screen_set_color(45);
    screen_put_str(5, 19, "[2] Ice        (blue)");
    screen_set_color(202);
    screen_put_str(5, 20, "[3] Lava       (red/orange)");
    screen_set_color(196);
    screen_put_str(5, 21, "[4] Rainbow    (multicolor)");
    screen_reset_color();

    screen_flush();

    while (1) {
        key = read_key();
        if (key == '1') return 0;
        if (key == '2') return 1;
        if (key == '3') return 2;
        if (key == '4') return 3;
        usleep(10000);
    }
}

void show_pause_overlay(Board *b) {
    int cx = OFFSET_X + my_divide(b->width, 2) - 3;
    int cy = OFFSET_Y + my_divide(b->height, 2);

    screen_set_color(226);  /* yellow */
    screen_put_str(cx, cy, "PAUSED");
    screen_put_str(cx - 4, cy + 1, "Press P to resume");
    screen_reset_color();
    screen_flush();
}

void show_game_over(Snake *s, Board *b, Score *sc) {
    SnakeSegment *seg;
    int flash;
    int i;

    (void)sc;
    (void)b;

    /* flash snake red 3 times */
    for (flash = 0; flash < 3; flash++) {
        /* red */
        screen_set_color(196);
        seg = s->head;
        while (seg) {
            screen_put_char(OFFSET_X + seg->x, OFFSET_Y + seg->y,
                           seg == s->head ? '@' : 'O');
            seg = seg->next;
        }
        screen_reset_color();
        screen_flush();
        usleep(150000);

        /* dim */
        screen_set_color(240);
        seg = s->head;
        while (seg) {
            screen_put_char(OFFSET_X + seg->x, OFFSET_Y + seg->y,
                           seg == s->head ? '@' : 'O');
            seg = seg->next;
        }
        screen_reset_color();
        screen_flush();
        usleep(150000);
    }

    /* sequential disappearance from tail to head */
    for (i = s->length - 1; i >= 0; i--) {
        int j = 0;
        seg = s->head;
        while (seg && j < i) { seg = seg->next; j++; }
        if (seg) {
            screen_put_char(OFFSET_X + seg->x, OFFSET_Y + seg->y, ' ');
            screen_flush();
            usleep(50000);
        }
    }
}

int show_game_over_prompt(Board *b, Score *s, Stats *st) {
    int cx = OFFSET_X + my_divide(b->width, 2) - 4;
    int cy = OFFSET_Y + my_divide(b->height, 2);
    char score_str[16];
    char high_str[16];
    char line[64];
    char num_str[16];
    int key;

    screen_set_color(196);
    screen_put_str(cx, cy - 1, "GAME OVER");
    screen_reset_color();

    int_to_str(s->score, score_str);
    int_to_str(s->high_score, high_str);

    my_strcpy(line, "Score: ");
    my_strcat(line, score_str);
    my_strcat(line, "  High: ");
    my_strcat(line, high_str);
    screen_put_str(cx - 2, cy + 1, line);

    screen_put_str(cx - 4, cy + 3, "R: Restart  |  Q: Quit");

    /* lifetime stats */
    screen_set_color(240);
    screen_put_str(cx - 5, cy + 5, "--- Lifetime Stats ---");

    my_strcpy(line, "Games: ");
    int_to_str(st->games_played, num_str);
    my_strcat(line, num_str);
    my_strcat(line, "    Food: ");
    int_to_str(st->total_food, num_str);
    my_strcat(line, num_str);
    screen_put_str(cx - 5, cy + 6, line);

    my_strcpy(line, "Longest: ");
    int_to_str(st->longest_snake, num_str);
    my_strcat(line, num_str);
    my_strcat(line, "  Best Lvl: ");
    int_to_str(st->best_level, num_str);
    my_strcat(line, num_str);
    screen_put_str(cx - 5, cy + 7, line);
    screen_reset_color();

    screen_flush();

    while (1) {
        key = read_key();
        if (key == 'r' || key == 'R') return 'r';
        if (key == 'q' || key == 'Q') return 'q';
        usleep(16000);
    }
}

void show_victory(Score *s, Board *b) {
    int cx = OFFSET_X + my_divide(b->width, 2) - 4;
    int cy = OFFSET_Y + my_divide(b->height, 2);
    char score_str[16];

    screen_set_color(46);
    screen_put_str(cx, cy, "YOU WIN!");
    screen_reset_color();

    int_to_str(s->score, score_str);

    screen_put_str(cx - 2, cy + 2, "Final Score: ");
    /* position score string right after "Final Score: " (13 chars) */
    screen_put_str(cx + 11, cy + 2, score_str);
    screen_flush();
}
