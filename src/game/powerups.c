#include "powerups.h"
#include "../lib/math.h"
#include <stddef.h>

void powerups_init(Powerups *p) {
    if (!p) return;
    p->count = 0;
    p->spawn_cooldown = 40;
}

void powerups_clear(Powerups *p) {
    if (p) p->count = 0;
}

int powerups_find_at(const Powerups *p, int x, int y) {
    int i;
    if (!p) return -1;
    for (i = 0; i < p->count; i++) {
        if (p->zones[i].x == x && p->zones[i].y == y) return i;
    }
    return -1;
}

void powerups_remove_at(Powerups *p, int index) {
    if (!p || index < 0 || index >= p->count) return;
    p->zones[index] = p->zones[p->count - 1];
    p->count--;
}

int powerups_tick(Powerups *p) {
    int i;
    int expired = 0;

    if (!p) return 0;
    if (p->spawn_cooldown > 0) p->spawn_cooldown--;

    i = 0;
    while (i < p->count) {
        p->zones[i].glow_phase = (p->zones[i].glow_phase + 1) % 4;
        if (p->zones[i].ticks_remaining > 0) {
            p->zones[i].ticks_remaining--;
            if (p->zones[i].ticks_remaining == 0) {
                powerups_remove_at(p, i);
                expired = 1;
                continue;
            }
        }
        i++;
    }
    return expired;
}

static int cell_is_free(int x, int y, Snake *snake, Snake *ghost, Foods *foods,
                        Obstacles *obs, Powerups *p) {
    int i;
    if (snake_occupies_cell(snake, x, y)) return 0;
    if (ghost && snake_occupies_cell(ghost, x, y)) return 0;
    if (foods) {
        for (i = 0; i < foods->count; i++) {
            if (foods->pieces[i].x == x && foods->pieces[i].y == y) return 0;
        }
    }
    if (obs) {
        for (i = 0; i < obs->count; i++) {
            if (obs->items[i].x == x && obs->items[i].y == y) return 0;
        }
    }
    if (powerups_find_at(p, x, y) >= 0) return 0;
    return 1;
}

void powerups_maybe_spawn(Powerups *p, Snake *snake, Snake *ghost, Foods *foods,
                          Obstacles *obs, int board_w, int board_h, int inset,
                          int *seed) {
    int retries;
    int x, y;
    int play_w, play_h;
    Powerup *slot;

    if (!p) return;
    if (p->count >= MAX_POWERUPS) return;
    if (p->spawn_cooldown > 0) return;

    play_w = board_w - my_multiply(inset, 2);
    play_h = board_h - my_multiply(inset, 2);
    if (play_w < 4 || play_h < 2) return;

    for (retries = 0; retries < 200; retries++) {
        x = inset + my_mod(my_abs(pseudo_random(seed)), play_w);
        y = inset + my_mod(my_abs(pseudo_random(seed)), play_h);
        if (!cell_is_free(x, y, snake, ghost, foods, obs, p)) continue;

        slot = &p->zones[p->count];
        slot->x = x;
        slot->y = y;
        slot->type = (my_mod(my_abs(pseudo_random(seed)), 2) == 0)
                         ? POW_BOOST : POW_SLOW;
        slot->ticks_remaining = POWERUP_LIFETIME;
        slot->glow_phase = 0;
        p->count++;
        p->spawn_cooldown = 80 + my_mod(my_abs(pseudo_random(seed)), 80);
        return;
    }
}
