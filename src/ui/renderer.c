#include "renderer.h"
#include "themes.h"
#include "../game/speed_zones.h"
#include "../lib/screen.h"
#include "../lib/string.h"
#include "../lib/math.h"
#include <stddef.h>

/* border offset: game area starts at (2,2) to leave room for border */
#define OFFSET_X 2
#define OFFSET_Y 2

void render_foods(Foods *fs, Board *b, const Theme *t) {
    int i;
    if (!fs) return;
    for (i = 0; i < fs->count; i++) {
        if (fs->slot[i].x >= 0 && fs->slot[i].x < b->width
            && fs->slot[i].y >= 0 && fs->slot[i].y < b->height) {
            render_food(&fs->slot[i], b, t);
        }
    }
}

void render_speed_zones(const SpeedZones *sz, Board *b, const Theme *t) {
    int j, d;
    (void)b;
    (void)t;
    if (!sz) return;
    for (j = 0; j < sz->count; j++) {
        d = sz->items[j].delta_ms;
        screen_set_color(d < 0 ? 39 : 93);
        screen_put_char(OFFSET_X + sz->items[j].x, OFFSET_Y + sz->items[j].y, ':');
    }
    screen_reset_color();
}

void render_restore_cell(int x, int y, const SpeedZones *sz, Board *b, const Theme *t) {
    int d;
    (void)b;
    (void)t;
    d = speed_zones_delta_at(sz, x, y);
    if (d != 0) {
        screen_set_color(d < 0 ? 39 : 93);
        screen_put_char(OFFSET_X + x, OFFSET_Y + y, ':');
    } else {
        screen_put_char(OFFSET_X + x, OFFSET_Y + y, ' ');
    }
    screen_reset_color();
}

void render_replay_dot(int x, int y, const Theme *t, int on) {
    (void)t;
    if (!on) return;
    screen_set_color(240);
    screen_put_char(OFFSET_X + x, OFFSET_Y + y, '.');
    screen_reset_color();
}

void render_ghost(Snake *g, const Theme *t) {
    SnakeSegment *seg;
    int dist;

    if (!g || !g->head) return;
    dist = 0;
    for (seg = g->head; seg; seg = seg->next) {
        screen_set_color(t->ghost_color);
        screen_put_char(OFFSET_X + seg->x, OFFSET_Y + seg->y, dist == 0 ? 'G' : 'g');
        dist++;
    }
    screen_reset_color();
}

void render_full_frame(const Theme *t, Board *b, Snake *s, Snake *ghost, Foods *foods,
                      Obstacles *o, Score *sc, const SpeedZones *sz,
                      int rep_x, int rep_y, int show_repl) {
    int inb;

    inb = (show_repl && rep_x >= 0 && rep_x < b->width && rep_y >= 0 && rep_y < b->height);
    screen_clear();
    render_border(b, t);
    if (sz) render_speed_zones(sz, b, t);
    render_foods(foods, b, t);
    render_obstacles(o, b, t);
    if (inb) {
        render_replay_dot(rep_x, rep_y, t, 1);
    }
    if (ghost) render_ghost(ghost, t);
    if (s) render_snake(s, t);
    render_hud(sc, b);
    screen_flush();
}

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

void render_food(Food *f, Board *b, const Theme *t) {
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

    /* avoid leftover chars and reflow: clear whole line to end */
    screen_clear_line(hud_y);
    screen_put_str(OFFSET_X, hud_y, line);

    my_strcpy(line, "WASD/Arrows: Move  |  P: Pause  |  Q: Quit");
    screen_clear_line(hud_y + 1);
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
