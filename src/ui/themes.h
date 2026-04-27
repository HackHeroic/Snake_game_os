#ifndef THEMES_H
#define THEMES_H

typedef struct {
    int head_color;
    int body_colors[5];
    int food_color;
    int bonus_food_color;
    int slow_food_color;
    int obstacle_color;
    int border_color;
    int ghost_color;      /* AI ghost snake */
    int rainbow;          /* 1 = use rainbow body logic, 0 = use gradient */
} Theme;

/* rainbow body colors: {196, 208, 226, 46, 21, 93} */
#define NUM_RAINBOW_COLORS 6

const Theme *theme_get(int index);
int theme_get_rainbow_color(int segment_index);

#endif
