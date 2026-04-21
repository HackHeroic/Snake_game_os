#include "renderer.h"
#include "themes.h"
#include "../lib/screen.h"
#include "../lib/string.h"
#include "../lib/math.h"
#include <stddef.h>

/* border offset: game area starts at (2,2) to leave room for border */
#define OFFSET_X 2
#define OFFSET_Y 2

void render_border(Board *b, const Theme *t) {
    int i;

    screen_set_color(t->border_color);

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

void render_snake(Snake *s, const Theme *t) {
    SnakeSegment *seg = s->head;
    int dist = 0;
    int color;

    while (seg != NULL) {
        if (dist == 0) {
            screen_set_color(t->head_color);
            screen_put_char(OFFSET_X + seg->x, OFFSET_Y + seg->y, '@');
        } else {
            if (t->rainbow) {
                color = theme_get_rainbow_color(dist);
            } else {
                color = t->body_colors[my_min(dist - 1, 4)];
            }
            screen_set_color(color);
            screen_put_char(OFFSET_X + seg->x, OFFSET_Y + seg->y, 'O');
        }

        dist++;
        seg = seg->next;
    }

    screen_reset_color();
}

void render_food(const Food *f, Board *b, const Theme *t) {
    int color;
    char ch;

    (void)b;

    if (f->type == FOOD_BONUS) {
        color = t->bonus_food_color;
        ch = '$';
    } else if (f->type == FOOD_SLOW) {
        color = t->slow_food_color;
        ch = '~';
    } else {
        color = t->food_color;
        ch = '*';
    }

    screen_set_color(color);
    screen_put_char(OFFSET_X + f->x, OFFSET_Y + f->y, ch);
    screen_reset_color();
}

void render_foods(const Foods *fs, Board *b, const Theme *t) {
    int i;

    if (!fs) return;
    for (i = 0; i < fs->count; i++) {
        render_food(&fs->pieces[i], b, t);
    }
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

void render_obstacles(Obstacles *obs, Board *b, const Theme *t) {
    int i;
    (void)b;

    screen_set_color(t->obstacle_color);
    for (i = 0; i < obs->count; i++) {
        screen_put_char(OFFSET_X + obs->items[i].x,
                       OFFSET_Y + obs->items[i].y, '#');
    }
    screen_reset_color();
}
