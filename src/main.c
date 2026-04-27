#include "lib/memory.h"
#include "lib/screen.h"
#include "lib/keyboard.h"
#include "lib/math.h"
#include "game/snake.h"
#include "game/food.h"
#include "game/board.h"
#include "game/score.h"
#include "game/obstacles.h"
#include "game/stats.h"
#include "game/ghost.h"
#include "game/powerups.h"
#include "game/replay.h"
#include "ui/renderer.h"
#include "ui/screens.h"
#include "ui/themes.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define OFFSET_X 2
#define OFFSET_Y 2

static int food_placement_invalid(Food *f, Snake *snake, Snake *ghost, Obstacles *obs,
                                  Board *board) {
    SnakeSegment *seg;
    int i;

    if (!f) return 0;
    if (!board_in_play_area(board, f->x, f->y)) return 1;

    for (seg = snake->head; seg != NULL; seg = seg->next) {
        if (seg->x == f->x && seg->y == f->y) return 1;
    }
    if (ghost && snake_occupies_cell(ghost, f->x, f->y)) return 1;
    for (i = 0; i < obs->count; i++) {
        if (obs->items[i].x == f->x && obs->items[i].y == f->y) return 1;
    }
    return 0;
}

static int foods_any_invalid(Foods *fs, Snake *snake, Snake *ghost, Obstacles *obs,
                             Board *board) {
    int i;
    for (i = 0; i < fs->count; i++) {
        if (food_placement_invalid(&fs->pieces[i], snake, ghost, obs, board)) return 1;
    }
    return 0;
}

static void powerups_drop_invalid(Powerups *p, Board *board) {
    int i;
    if (!p) return;
    i = 0;
    while (i < p->count) {
        if (!board_in_play_area(board, p->zones[i].x, p->zones[i].y)) {
            powerups_remove_at(p, i);
        } else {
            i++;
        }
    }
}

static void redraw_full_game(Board *board, Snake *snake, Snake *ghost, Foods *foods,
                             Obstacles *obs, Powerups *pups, Score *score,
                             const Theme *theme) {
    (void)obs;

    screen_clear();
    render_border(board, theme);
    render_powerups(pups, board, theme);
    render_foods(foods, board, theme);
    if (ghost) render_ghost_snake(ghost, theme);
    render_snake(snake, theme);
    render_hud(score, board);
    screen_flush();
}

/* Wait until terminal meets minimum size or user quits. Returns 0 ok, -1 quit. */
static int wait_for_valid_terminal(void) {
    int key;

    for (;;) {
        if (get_terminal_width() >= MIN_TERMINAL_COLS &&
            get_terminal_height() >= MIN_TERMINAL_ROWS) return 0;

        screen_clear();
        screen_put_str(2, 2, "Terminal too small. Resize or press Q.");
        screen_flush();

        key = read_key();
        if (key == 'q' || key == 'Q') return -1;
        usleep(50000);
    }
}

static int apply_terminal_resize(int *board_w, int *board_h, Board *board,
                                 Snake *snake, Snake **ghost, Foods *foods,
                                 Obstacles *obs, Powerups *pups, Score *score,
                                 const Theme *theme, int *seed, int paused) {
    int tw, th;
    Snake *g;

    terminal_sync_winsize();
    tw = get_terminal_width();
    th = get_terminal_height();

    if (tw < MIN_TERMINAL_COLS || th < MIN_TERMINAL_ROWS) {
        if (wait_for_valid_terminal() != 0) return -1;
        tw = get_terminal_width();
        th = get_terminal_height();
    }

    *board_w = tw - 4;
    *board_h = th - 6;
    board_set_size(board, *board_w, *board_h);

    g = ghost ? *ghost : NULL;
    if (ghost && *ghost) {
        if (!snakes_refit_inset(snake, *ghost, *board_w, *board_h, board->inset)) {
            if (!snakes_refit_inset(snake, NULL, *board_w, *board_h, board->inset)) {
                snake->alive = 0;
                return 1;
            }
            snake_free(*ghost);
            *ghost = ghost_create(snake, foods, obs, *board_w, *board_h,
                                   board->inset, seed);
        }
        g = *ghost;
        if (g && snake_snakes_overlap(snake, g)) {
            snake->alive = 0;
            return 1;
        }
    } else {
        if (!snakes_refit_inset(snake, NULL, *board_w, *board_h, board->inset)) {
            snake->alive = 0;
            return 1;
        }
    }

    powerups_drop_invalid(pups, board);

    if (foods_any_invalid(foods, snake, g, obs, board)) {
        foods_clear(foods);
        foods_fill_to_target(foods, snake, g, obs, *board_w, *board_h, board->inset,
                              seed, foods_target_count(score->level));
        if (foods->count == 0) {
            snake->alive = 0;
            return 1;
        }
    }

    redraw_full_game(board, snake, g, foods, obs, pups, score, theme);
    if (paused) show_pause_overlay(board);
    return 0;
}

/* Apply level-driven shrink. Returns 1 if inset changed (full redraw needed). */
static int apply_level_shrink(Board *board, Snake *snake, Snake **ghost,
                              Foods *foods, Obstacles *obs, Powerups *pups,
                              Score *score, int board_w, int board_h, int *seed) {
    int new_inset;
    Snake *g;

    new_inset = board_inset_for_level(board_w, board_h, score->level);
    if (new_inset == board->inset) return 0;
    if (new_inset < board->inset) {
        /* growing back not used; only ever shrinks */
        return 0;
    }

    board_set_inset(board, new_inset);

    g = ghost ? *ghost : NULL;
    if (g) {
        if (!snakes_refit_inset(snake, g, board_w, board_h, board->inset)) {
            snake_free(g);
            *ghost = NULL;
            g = NULL;
            if (!snakes_refit_inset(snake, NULL, board_w, board_h, board->inset)) {
                snake->alive = 0;
                return 1;
            }
        }
    } else {
        if (!snakes_refit_inset(snake, NULL, board_w, board_h, board->inset)) {
            snake->alive = 0;
            return 1;
        }
    }

    powerups_drop_invalid(pups, board);

    if (foods_any_invalid(foods, snake, g, obs, board)) {
        foods_clear(foods);
        foods_fill_to_target(foods, snake, g, obs, board_w, board_h, board->inset,
                              seed, foods_target_count(score->level));
    }

    return 1;
}

int main(void) {
    int seed = 0;
    int mode;
    int theme_idx;
    const Theme *theme;
    int running = 1;
    Board *board;
    Snake *snake;
    Foods foods;
    Score *score;
    Obstacles obstacles;
    Stats stats;
    Powerups powerups;
    Replay record;
    Replay best;
    int has_best;
    int term_w, term_h;
    int board_w, board_h;
    int paused;
    int key;
    int nx, ny;
    int grew;
    int food_idx;
    FoodType ate_type;
    int old_tail_x, old_tail_y;
    int last_score;
    int prev_level;
    int slow_ticks;
    int boost_ticks;
    int speed;
    int points;
    int target_food;
    Snake *ghost;
    int ghost_tail_x, ghost_tail_y;
    int ghost_moved;
    int pup_idx;
    int replay_step;
    int replay_prev_x, replay_prev_y;

    memory_init();
    keyboard_init();
    terminal_install_winch_handler();
    screen_hide_cursor();

    stats_load(&stats);

    while (running) {
        mode = show_title_screen(&seed);
        theme_idx = show_theme_screen();
        theme = theme_get(theme_idx);

        term_w = get_terminal_width();
        term_h = get_terminal_height();

        if (term_w < MIN_TERMINAL_COLS || term_h < MIN_TERMINAL_ROWS) {
            keyboard_restore();
            screen_show_cursor();
            screen_clear();
            printf("Terminal too small! Need at least %dx%d, got %dx%d\n",
                   MIN_TERMINAL_COLS, MIN_TERMINAL_ROWS, term_w, term_h);
            return 1;
        }

        board_w = term_w - 4;
        board_h = term_h - 6;

        board = board_create(board_w, board_h, mode);
        snake = snake_create(my_divide(board_w, 2), my_divide(board_h, 2));
        score = score_create();
        score_load_high(score);
        obstacles_init(&obstacles);
        foods_init(&foods);
        powerups_init(&powerups);

        target_food = foods_target_count(score->level);
        foods_fill_to_target(&foods, snake, NULL, &obstacles, board_w, board_h,
                             board->inset, &seed, target_food);

        ghost = ghost_create(snake, &foods, &obstacles, board_w, board_h,
                             board->inset, &seed);

        has_best = replay_load_best(&best);
        replay_begin_record(&record, board_w, board_h);
        replay_step = 0;
        replay_prev_x = -1;
        replay_prev_y = -1;

        screen_clear();
        render_border(board, theme);
        render_powerups(&powerups, board, theme);
        render_foods(&foods, board, theme);
        if (ghost) render_ghost_snake(ghost, theme);
        render_snake(snake, theme);
        render_hud(score, board);
        screen_flush();

        paused = 0;
        last_score = 0;
        prev_level = 0;
        slow_ticks = 0;
        boost_ticks = 0;

        while (snake->alive) {
            if (terminal_consume_winch() || !board_matches_terminal(board)) {
                int ar = apply_terminal_resize(&board_w, &board_h, board, snake, &ghost,
                                              &foods, &obstacles, &powerups, score,
                                              theme, &seed, paused);
                if (ar == -1) {
                    running = 0;
                    snake->alive = 0;
                    break;
                }
                if (!snake->alive) break;
                if (ar == 0) last_score = score->score;
                continue;
            }

            key = read_key();

            if (key == 'q' || key == 'Q') {
                snake->alive = 0;
                running = 0;
                break;
            }
            if (key == 'p' || key == 'P') {
                paused = !paused;
                if (paused) {
                    show_pause_overlay(board);
                } else {
                    redraw_full_game(board, snake, ghost, &foods, &obstacles,
                                     &powerups, score, theme);
                }
                continue;
            }
            if (paused) {
                usleep(50000);
                continue;
            }

            if (key == 'w' || key == 'W' || key == KEY_UP)
                snake_set_direction(snake, DIR_UP);
            else if (key == 's' || key == 'S' || key == KEY_DOWN)
                snake_set_direction(snake, DIR_DOWN);
            else if (key == 'a' || key == 'A' || key == KEY_LEFT)
                snake_set_direction(snake, DIR_LEFT);
            else if (key == 'd' || key == 'D' || key == KEY_RIGHT)
                snake_set_direction(snake, DIR_RIGHT);

            snake_compute_next_head(snake, &nx, &ny);

            if (board->mode == 0) {
                if (board_check_wall_collision(board, nx, ny)) {
                    snake->alive = 0;
                    break;
                }
            } else {
                board_wrap_position(board, &nx, &ny);
            }

            if (snake_check_self_collision(snake, nx, ny)) {
                snake->alive = 0;
                break;
            }

            food_idx = foods_find_at(&foods, nx, ny);
            grew = (food_idx >= 0);
            if (grew) {
                ate_type = foods.pieces[food_idx].type;
            }

            old_tail_x = snake->tail->x;
            old_tail_y = snake->tail->y;

            snake_move(snake, nx, ny, grew);

            /* Record head position for replay */
            replay_record_frame(&record, nx, ny);

            /* Check powerup zone hit */
            pup_idx = powerups_find_at(&powerups, nx, ny);
            if (pup_idx >= 0) {
                if (powerups.zones[pup_idx].type == POW_BOOST) {
                    boost_ticks = POWERUP_EFFECT_TICKS;
                    slow_ticks = 0;
                } else {
                    slow_ticks = POWERUP_EFFECT_TICKS;
                    boost_ticks = 0;
                }
                render_powerup_clear(powerups.zones[pup_idx].x,
                                     powerups.zones[pup_idx].y);
                powerups_remove_at(&powerups, pup_idx);
            }

            if (grew) {
                foods_remove_at(&foods, food_idx);

                points = (ate_type == FOOD_BONUS) ? 3 : 1;

                if (ate_type == FOOD_SLOW) {
                    slow_ticks = 30;
                }

                score_increment(score, points);
                stats_on_food(&stats, snake->length);

                if (score->level > prev_level) {
                    stats_on_level(&stats, score->level);
                    prev_level = score->level;
                    if (apply_level_shrink(board, snake, &ghost, &foods, &obstacles,
                                           &powerups, score, board_w, board_h, &seed)) {
                        if (!snake->alive) break;
                        redraw_full_game(board, snake, ghost, &foods, &obstacles,
                                         &powerups, score, theme);
                        last_score = score->score;
                    }
                }

                target_food = foods_target_count(score->level);
                foods_fill_to_target(&foods, snake, ghost, &obstacles, board_w, board_h,
                                     board->inset, &seed, target_food);
                if (foods.count == 0) {
                    show_victory(score, board);
                    score_save_high(score);
                    stats_on_game_over(&stats);
                    stats_save(&stats);
                    replay_save_if_best(&record, score->score);
                    usleep(3000000);
                    break;
                }
                render_foods(&foods, board, theme);
            } else {
                render_erase_tail(old_tail_x, old_tail_y, board);
            }

            if (ghost && snake_snakes_overlap(snake, ghost)) {
                snake->alive = 0;
                break;
            }

            /* Food expiry pass (timed bonus/slow) */
            {
                int i = 0;
                int expired_any = 0;
                target_food = foods_target_count(score->level);
                while (i < foods.count) {
                    if (foods.pieces[i].ticks_remaining > 0) {
                        foods.pieces[i].ticks_remaining--;
                        if (foods.pieces[i].ticks_remaining == 0) {
                            screen_put_char(OFFSET_X + foods.pieces[i].x,
                                           OFFSET_Y + foods.pieces[i].y, ' ');
                            foods_remove_at(&foods, i);
                            expired_any = 1;
                            continue;
                        }
                    }
                    i++;
                }
                if (expired_any) {
                    foods_fill_to_target(&foods, snake, ghost, &obstacles, board_w,
                                         board_h, board->inset, &seed, target_food);
                    render_foods(&foods, board, theme);
                }
            }

            /* Powerup tick + maybe spawn */
            if (powerups_tick(&powerups)) {
                redraw_full_game(board, snake, ghost, &foods, &obstacles,
                                 &powerups, score, theme);
            }
            powerups_maybe_spawn(&powerups, snake, ghost, &foods, &obstacles,
                                 board_w, board_h, board->inset, &seed);

            ghost_moved = 0;
            if (ghost) {
                ghost_tail_x = ghost->tail->x;
                ghost_tail_y = ghost->tail->y;
                ghost_moved = ghost_step(ghost, board, &seed);
                if (ghost_moved)
                    render_erase_tail(ghost_tail_x, ghost_tail_y, board);
                if (snake_snakes_overlap(snake, ghost)) {
                    snake->alive = 0;
                    break;
                }
            }

            /* Replay ghost: clear previous, draw current step */
            if (has_best && replay_step < best.count) {
                if (replay_prev_x >= 0 &&
                    board_in_play_area(board, replay_prev_x, replay_prev_y) &&
                    !snake_occupies_cell(snake, replay_prev_x, replay_prev_y) &&
                    (!ghost || !snake_occupies_cell(ghost, replay_prev_x, replay_prev_y)) &&
                    foods_find_at(&foods, replay_prev_x, replay_prev_y) < 0 &&
                    powerups_find_at(&powerups, replay_prev_x, replay_prev_y) < 0) {
                    render_replay_ghost_clear(replay_prev_x, replay_prev_y);
                }
                replay_prev_x = best.frames[replay_step].x;
                replay_prev_y = best.frames[replay_step].y;
                if (board_in_play_area(board, replay_prev_x, replay_prev_y)) {
                    render_replay_ghost(replay_prev_x, replay_prev_y, theme);
                }
                replay_step++;
            }

            render_powerups(&powerups, board, theme);
            if (ghost) render_ghost_snake(ghost, theme);
            render_snake(snake, theme);
            if (score->score != last_score) {
                render_hud(score, board);
                last_score = score->score;
            }
            screen_flush();

            speed = score_get_speed(score);
            if (boost_ticks > 0) {
                speed = my_max(40, speed - 70);
                boost_ticks--;
            }
            if (slow_ticks > 0) {
                speed += 50;
                slow_ticks--;
            }
            usleep(my_multiply(speed, 1000));
        }

        if (running && !snake->alive) {
            stats_on_game_over(&stats);
            stats_save(&stats);
            replay_save_if_best(&record, score->score);
            show_game_over(snake, board, score);
            score_save_high(score);

            key = show_game_over_prompt(board, score, &stats);
            if (key == 'q') {
                running = 0;
            }
        }

        if (!running) {
            stats_save(&stats);
            replay_save_if_best(&record, score->score);
        }

        snake_free(snake);
        if (ghost) snake_free(ghost);
        score_free(score);
        board_free(board);
    }

    keyboard_restore();
    screen_show_cursor();
    screen_reset_color();
    screen_clear();

    return 0;
}
