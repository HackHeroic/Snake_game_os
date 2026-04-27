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
#include "game/speed_zones.h"
#include "game/replay.h"
#include "ui/renderer.h"
#include "ui/screens.h"
#include "ui/themes.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define OFFSET_X 2
#define OFFSET_Y 2

static void show_terminal_too_small(void) {
    screen_clear();
    screen_put_str(0, 0,
        "Window too small (min 22x14). Enlarge, or Q to quit.                    ");
    screen_flush();
}

int main(void) {
    int seed = 0;
    int mode;
    int theme_idx;
    const Theme *theme;
    int running = 1;
    Board *board;
    Snake *snake;
    Snake *ghost;
    Foods foods;
    Score *score;
    Obstacles obstacles;
    Stats stats;
    int term_w, term_h;
    int board_w, board_h;
    int term_stalled;
    int paused;
    int key;
    int nx, ny;
    int grew;
    int old_tail_x, old_tail_y;
    int last_score;
    int prev_level;
    int slow_ticks;
    int speed;
    int points;
    int eat_idx;
    int old_ghost_tx, old_ghost_ty;
    int zone_delta;
    int shrink_counter;
    SpeedZones szones;
    Replay run_replay;
    Replay best_replay;
    int best_replay_stored;
    int replay_play_tick;
    int prev_rep_x, prev_rep_y;
    int prev_rep_on;
    int rgx, rgy, rep_on;
    int head_from_x, head_from_y;

    /* init systems */
    memory_init();
    keyboard_init();
    screen_hide_cursor();

    /* load lifetime stats once */
    stats_load(&stats);
    replay_load_best(&best_replay, &best_replay_stored);

    while (running) {
        /* title screen — also seeds PRNG */
        mode = show_title_screen(&seed);

        /* theme selection */
        theme_idx = show_theme_screen();
        theme = theme_get(theme_idx);

        /* compute board from terminal size */
        term_w = get_terminal_width();
        term_h = get_terminal_height();

        if (term_w < BOARD_MIN_TERM_W || term_h < BOARD_MIN_TERM_H) {
            keyboard_restore();
            screen_show_cursor();
            screen_clear();
            printf("Terminal too small! Need at least %dx%d, got %dx%d\n",
                   BOARD_MIN_TERM_W, BOARD_MIN_TERM_H, term_w, term_h);
            return 1;
        }

        /* board is terminal minus borders and HUD space */
        board_w = term_w - 4;   /* 2 for border + 2 margin */
        board_h = term_h - 6;   /* 2 for border + 2 HUD lines + 2 margin */

        /* create game objects */
        board = board_create(board_w, board_h, mode);
        snake = snake_create(my_divide(board_w, 2), my_divide(board_h, 2));
        ghost = snake_create(my_divide(my_multiply(board_w, 2), 3), my_divide(board_h, 2));
        score = score_create();
        score_load_high(score);
        obstacles_init(&obstacles);
        foods_init(&foods);
        foods_maintain(&foods, score, snake, &obstacles, board_w, board_h, &seed);
        speed_zones_init(&szones, board_w, board_h, snake, &foods, &obstacles, &seed);
        replay_clear(&run_replay);
        replay_play_tick = 0;
        prev_rep_on = 0;
        shrink_counter = 0;

        /* initial draw */
        render_full_frame(theme, board, snake, ghost, &foods, &obstacles, score, &szones, 0, 0, 0);

        paused = 0;
        term_stalled = 0;
        last_score = 0;
        prev_level = 0;
        slow_ticks = 0;

        /* game loop */
        while (snake->alive) {
            if (keyboard_consume_resize() || term_stalled) {
                term_w = get_terminal_width();
                term_h = get_terminal_height();
                if (term_w < BOARD_MIN_TERM_W || term_h < BOARD_MIN_TERM_H) {
                    term_stalled = 1;
                    show_terminal_too_small();
                    key = read_key();
                    if (key == 'q' || key == 'Q') {
                        running = 0;
                        snake->alive = 0;
                        break;
                    }
                    usleep(50000);
                    continue;
                }
                term_stalled = 0;
                board_w = term_w - 4;
                board_h = term_h - 6;
                board_set_size(board, board_w, board_h);
                snake_fit_board(snake, board_w, board_h);
                if (!snake->alive) {
                    break;
                }
                snake_fit_board(ghost, board_w, board_h);
                if (!ghost->alive) {
                    snake_free(ghost);
                    ghost = snake_create(my_divide(my_multiply(board_w, 2), 3), my_divide(board_h, 2));
                }
                obstacles_cull_outside(&obstacles, board_w, board_h);
                foods_cull_outside(&foods, board_w, board_h);
                foods_maintain(&foods, score, snake, &obstacles, board_w, board_h, &seed);
                speed_zones_cull(&szones, board_w, board_h);
                if (szones.count < 3) {
                    speed_zones_init(&szones, board_w, board_h, snake, &foods, &obstacles, &seed);
                }
                render_full_frame(theme, board, snake, ghost, &foods, &obstacles, score, &szones, 0, 0, 0);
                if (paused) {
                    show_pause_overlay(board);
                }
            }

            key = read_key();

            /* handle controls */
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
                    render_full_frame(theme, board, snake, ghost, &foods, &obstacles, score, &szones, 0, 0, 0);
                }
                continue;
            }
            if (paused) {
                usleep(50000);
                continue;
            }

            if (!paused) {
                shrink_counter++;
                if (shrink_counter >= 450 && board_w > 14 && board_h > 10) {
                    shrink_counter = 0;
                    board_w--;
                    board_h--;
                    board_set_size(board, board_w, board_h);
                    snake_fit_board(snake, board_w, board_h);
                    snake_fit_board(ghost, board_w, board_h);
                    if (!ghost->alive) {
                        snake_free(ghost);
                        ghost = snake_create(my_divide(my_multiply(board_w, 2), 3), my_divide(board_h, 2));
                    }
                    foods_cull_outside(&foods, board_w, board_h);
                    foods_maintain(&foods, score, snake, &obstacles, board_w, board_h, &seed);
                    obstacles_cull_outside(&obstacles, board_w, board_h);
                    speed_zones_cull(&szones, board_w, board_h);
                    if (szones.count < 3) {
                        speed_zones_init(&szones, board_w, board_h, snake, &foods, &obstacles, &seed);
                    }
                }
            }

            old_ghost_tx = ghost->tail->x;
            old_ghost_ty = ghost->tail->y;
            ghost_step(ghost, board, &obstacles, &foods, snake, &seed);
            if (ghost->head->x == snake->head->x
                && ghost->head->y == snake->head->y) {
                snake->alive = 0;
                break;
            }

            /* map input to direction */
            if (key == 'w' || key == 'W' || key == KEY_UP)
                snake_set_direction(snake, DIR_UP);
            else if (key == 's' || key == 'S' || key == KEY_DOWN)
                snake_set_direction(snake, DIR_DOWN);
            else if (key == 'a' || key == 'A' || key == KEY_LEFT)
                snake_set_direction(snake, DIR_LEFT);
            else if (key == 'd' || key == 'D' || key == KEY_RIGHT)
                snake_set_direction(snake, DIR_RIGHT);

            /* 1. compute candidate position */
            snake_compute_next_head(snake, &nx, &ny);

            /* 2. wall check / wrap */
            if (board->mode == 0) {
                /* classic mode */
                if (board_check_wall_collision(board, nx, ny)) {
                    snake->alive = 0;
                    break;
                }
            } else {
                /* wrap mode */
                board_wrap_position(board, &nx, &ny);
            }

            /* 3. self-collision check */
            if (snake_check_self_collision(snake, nx, ny)) {
                snake->alive = 0;
                break;
            }

            /* 4. obstacle collision check */
            if (obstacles_check_collision(&obstacles, nx, ny)) {
                snake->alive = 0;
                break;
            }

            if (snake_occupies(ghost, nx, ny)) {
                snake->alive = 0;
                break;
            }

            /* 5. food check (before move) */
            eat_idx = foods_find_at(&foods, nx, ny);
            grew = (eat_idx >= 0);

            /* save old tail for erasing */
            old_tail_x = snake->tail->x;
            old_tail_y = snake->tail->y;

            /* cell we leave (for speed zone tick that tracks the step) */
            head_from_x = snake->head->x;
            head_from_y = snake->head->y;

            /* 6. execute move */
            snake_move(snake, nx, ny, grew);

            /* 7. post-move updates */
            if (grew) {
                Food *eatf = &foods.slot[eat_idx];

                points = (eatf->type == FOOD_BONUS) ? 3 : 1;
                if (eatf->type == FOOD_SLOW) {
                    slow_ticks = 30;
                }
                score_increment(score, points);
                stats_on_food(&stats, snake->length);
                foods_remove_at(&foods, eat_idx);
                if (score->level > prev_level) {
                    stats_on_level(&stats, score->level);
                    obstacles_spawn(&obstacles, snake, &foods,
                                    board_w, board_h, &seed);
                    render_obstacles(&obstacles, board, theme);
                    prev_level = score->level;
                }
                foods_maintain(&foods, score, snake, &obstacles, board_w, board_h, &seed);
                if (foods.count == 0) {
                    if (!foods_try_add(&foods, snake, &obstacles, board_w, board_h, &seed)) {
                        if (snake->length >= my_multiply(board_w, board_h) - 1) {
                            replay_push(&run_replay, snake->head->x, snake->head->y);
                            show_victory(score, board);
                            score_save_high(score);
                            stats_on_game_over(&stats);
                            stats_save(&stats);
                            usleep(3000000);
                            break;
                        }
                    }
                }
                {
                    int k;
                    for (k = 0; k < foods.count; k++) {
                        render_food(&foods.slot[k], board, theme);
                    }
                }
            } else {
                render_erase_tail(old_tail_x, old_tail_y, board);
            }

            /* 8. food despawn countdown */
            {
                int i, rem;
                i = 0;
                while (i < foods.count) {
                    if (foods.slot[i].ticks_remaining > 0) {
                        foods.slot[i].ticks_remaining--;
                        if (foods.slot[i].ticks_remaining == 0) {
                            rem = i;
                            render_restore_cell(foods.slot[rem].x, foods.slot[rem].y, &szones, board, theme);
                            foods_remove_at(&foods, rem);
                            foods_maintain(&foods, score, snake, &obstacles, board_w, board_h, &seed);
                            continue;
                        }
                    }
                    i++;
                }
            }

            /* 9. render and wait */
            if (prev_rep_on) {
                render_restore_cell(prev_rep_x, prev_rep_y, &szones, board, theme);
            }
            render_restore_cell(old_ghost_tx, old_ghost_ty, &szones, board, theme);
            if (!grew) {
                render_restore_cell(old_tail_x, old_tail_y, &szones, board, theme);
            }
            rep_on = 0;
            if (best_replay.len > 0 && replay_play_tick < best_replay.len) {
                if (replay_step_coords(&best_replay, replay_play_tick, &rgx, &rgy)) {
                    if (rgx >= 0 && rgx < board_w && rgy >= 0 && rgy < board_h) {
                        rep_on = 1;
                        render_replay_dot(rgx, rgy, theme, 1);
                    }
                }
            }
            replay_play_tick++;
            if (rep_on) {
                prev_rep_x = rgx;
                prev_rep_y = rgy;
            }
            prev_rep_on = rep_on;

            render_ghost(ghost, theme);
            render_snake(snake, theme);
            if (score->score != last_score) {
                render_hud(score, board);
                last_score = score->score;
            }
            screen_flush();

            replay_push(&run_replay, snake->head->x, snake->head->y);

            /* apply speed with optional slow effect */
            speed = score_get_speed(score);
            if (slow_ticks > 0) {
                speed += 50;
                slow_ticks--;
            }
            /* Use head after move (authoritative) so delay matches the cell the snake is on. */
            zone_delta = speed_zones_delta_for_step(
                &szones, head_from_x, head_from_y, snake->head->x, snake->head->y);
            speed += zone_delta;
            if (speed < 40) speed = 40;
            if (speed > 400) speed = 400;
            usleep(my_multiply(speed, 1000));
        }
        replay_try_save_best(&run_replay, score->score);
        replay_load_best(&best_replay, &best_replay_stored);

        /* game over sequence (only if not quitting) */
        if (running && !snake->alive) {
            if (ghost) {
                snake_free(ghost);
                ghost = NULL;
            }
            stats_on_game_over(&stats);
            stats_save(&stats);
            if (show_game_over(theme, snake, board, &foods, &obstacles, score) == 0) {
                score_save_high(score);
                key = show_game_over_prompt(theme, board, score, &stats, &foods, &obstacles);
                if (key == 'q') {
                    running = 0;
                }
            } else {
                running = 0;
            }
        }

        /* save stats on quit (not just game over) */
        if (!running) {
            stats_save(&stats);
        }

        /* cleanup for restart or exit */
        if (ghost) {
            snake_free(ghost);
            ghost = NULL;
        }
        snake_free(snake);
        score_free(score);
        board_free(board);
    }

    /* final cleanup */
    keyboard_restore();
    screen_show_cursor();
    screen_reset_color();
    screen_clear();

    return 0;
}
