#include "renderer.h"
#include "../lib/screen.h"
#include "../lib/string.h"
#include "../lib/math.h"
#include <stddef.h>

/* border offset: game area starts at (2,2) to leave room for border */
#define OFFSET_X 2
#define OFFSET_Y 2

/* green gradient colors for snake body (256-color codes) */
static const int snake_colors[] = { 46, 40, 34, 28, 22 };
#define NUM_SNAKE_COLORS 5

void render_border(Board *b) {
    int i;

    screen_set_color(45);  /* cyan */

    /* top border */
    screen_put_char(OFFSET_X - 1, OFFSET_Y - 1, '+');
    for (i = 0; i < b->width; i++) {
        screen_put_char(OFFSET_X + i, OFFSET_Y - 1, '-');
    }
    screen_put_char(OFFSET_X + b->width, OFFSET_Y - 1, '+');

    /* bottom border */
    screen_put_char(OFFSET_X - 1, OFFSET_Y + b->height, '+');
    for (i = 0; i < b->width; i++) {
        screen_put_char(OFFSET_X + i, OFFSET_Y + b->height, '-');
    }
    screen_put_char(OFFSET_X + b->width, OFFSET_Y + b->height, '+');

    /* side borders */
    for (i = 0; i < b->height; i++) {
        screen_put_char(OFFSET_X - 1, OFFSET_Y + i, '|');
        screen_put_char(OFFSET_X + b->width, OFFSET_Y + i, '|');
    }

    screen_reset_color();
}

void render_snake(Snake *s) {
    SnakeSegment *seg = s->head;
    int dist = 0;
    int color_idx;

    while (seg != NULL) {
        color_idx = my_min(dist, NUM_SNAKE_COLORS - 1);
        screen_set_color(snake_colors[color_idx]);

        if (dist == 0) {
            screen_put_char(OFFSET_X + seg->x, OFFSET_Y + seg->y, '@');
        } else {
            screen_put_char(OFFSET_X + seg->x, OFFSET_Y + seg->y, 'O');
        }

        dist++;
        seg = seg->next;
    }

    screen_reset_color();
}

void render_food(Food *f, Board *b) {
    (void)b;
    screen_set_color(196);  /* bright red */
    screen_put_char(OFFSET_X + f->x, OFFSET_Y + f->y, '*');
    screen_reset_color();
}

void render_hud(Score *s, Board *b) {
    char line[128];
    char score_str[16];
    char high_str[16];
    char level_str[16];
    int hud_y = OFFSET_Y + b->height + 1;

    int_to_str(s->score, score_str);
    int_to_str(s->high_score, high_str);
    int_to_str(s->level, level_str);

    /* build HUD line manually */
    my_strcpy(line, "Score: ");
    my_strcat(line, score_str);
    my_strcat(line, "  |  High: ");
    my_strcat(line, high_str);
    my_strcat(line, "  |  Level: ");
    my_strcat(line, level_str);
    my_strcat(line, "  |  Mode: ");
    my_strcat(line, b->mode ? "Wrap" : "Classic");

    /* clear the line first to prevent leftover chars */
    screen_put_str(OFFSET_X, hud_y, "                                                              ");
    screen_put_str(OFFSET_X, hud_y, line);

    my_strcpy(line, "WASD/Arrows: Move  |  P: Pause  |  Q: Quit");
    screen_put_str(OFFSET_X, hud_y + 1, line);
}

void render_erase_tail(int x, int y, Board *b) {
    (void)b;
    screen_put_char(OFFSET_X + x, OFFSET_Y + y, ' ');
}
