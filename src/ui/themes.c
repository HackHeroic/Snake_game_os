#include "themes.h"

static const int rainbow_colors[NUM_RAINBOW_COLORS] = {196, 208, 226, 46, 21, 93};

static const Theme themes[4] = {
    /* Classic */
    { 46, {46, 40, 34, 28, 22}, 196, 226, 39, 250, 45, 0 },
    /* Ice */
    { 45, {45, 39, 33, 27, 21}, 226, 214, 195, 255, 75, 0 },
    /* Lava */
    { 202, {202, 196, 160, 124, 88}, 46, 226, 33, 250, 208, 0 },
    /* Rainbow */
    { 196, {196, 208, 226, 46, 21}, 255, 226, 39, 250, 45, 1 }
};

const Theme *theme_get(int index) {
    if (index < 0 || index > 3) index = 0;
    return &themes[index];
}

int theme_get_rainbow_color(int segment_index) {
    int idx = segment_index % NUM_RAINBOW_COLORS;
    if (idx < 0) idx += NUM_RAINBOW_COLORS;
    return rainbow_colors[idx];
}
