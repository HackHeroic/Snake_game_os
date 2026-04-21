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
#include "ui/renderer.h"
#include "ui/screens.h"
#include "ui/themes.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define OFFSET_X 2
#define OFFSET_Y 2

static int food_placement_invalid(Food *f, Snake *snake, Obstacles *obs,
                                  int bw, int bh) {
    SnakeSegment *seg;
    int i;

    if (!f) return 0;
    if (f->x < 0 || f->x >= bw || f->y < 0 || f->y >= bh) return 1;

    for (seg = snake->head; seg != NULL; seg = seg->next) {
        if (seg->x == f->x && seg->y == f->y) return 1;
    }
    for (i = 0; i < obs->count; i++) {
        if (obs->items[i].x == f->x && obs->items[i].y == f->y) return 1;
    }
    return 0;
}

static int snake_on_any_obstacle(Snake *s, Obstacles *obs) {
    SnakeSegment *seg;

    for (seg = s->head; seg != NULL; seg = seg->next) {
        if (obstacles_check_collision(obs, seg->x, seg->y)) return 1;
    }
    return 0;
}

static void redraw_full_game(Board *board, Snake *snake, Food *food,
                             Obstacles *obs, Score *score, const Theme *theme) {
    screen_clear();
    render_border(board, theme);
    render_snake(snake, theme);
    if (food) render_food(food, board, theme);
    render_obstacles(obs, board, theme);
    render_hud(score, board);
    screen_flush();
}

/* Wait until terminal meets minimum size or user quits. Returns 0 ok, -1 quit. */
static int wait_for_valid_terminal(void) {
    int key;

    for (;;) {
        if (get_terminal_width() >= 22 && get_terminal_height() >= 14) return 0;

        screen_clear();
        screen_put_str(2, 2, "Terminal too small (need 22x14). Resize or press Q.");
        screen_flush();

        key = read_key();
        if (key == 'q' || key == 'Q') return -1;
        usleep(50000);
    }
}

/*
 * Recompute board from terminal, refit snake, clamp obstacles, fix food.
 * Returns: 0 ok, -1 user quit (Q while too small), 1 snake died (squashed/overlap).
 */
static int apply_terminal_resize(int *board_w, int *board_h, Board *board,
                                 Snake *snake, Food **food, Obstacles *obs,
                                 Score *score, const Theme *theme, int *seed,
                                 int paused) {
    int tw, th;
    Food *nf;

    tw = get_terminal_width();
    th = get_terminal_height();

    if (tw < 22 || th < 14) {
        if (wait_for_valid_terminal() != 0) return -1;
        tw = get_terminal_width();
        th = get_terminal_height();
    }

    *board_w = tw - 4;
    *board_h = th - 6;
    board_set_size(board, *board_w, *board_h);

    if (!snake_refit_to_board(snake, *board_w, *board_h)) {
        snake->alive = 0;
        return 1;
    }

    obstacles_clamp_to_board(obs, *board_w, *board_h);

    if (snake_on_any_obstacle(snake, obs)) {
        snake->alive = 0;
        return 1;
    }

    if (food_placement_invalid(*food, snake, obs, *board_w, *board_h)) {
        if (*food) {
            food_free(*food);
            *food = NULL;
        }
        nf = food_spawn(snake, obs, *board_w, *board_h, seed);
        if (nf == NULL) {
            snake->alive = 0;
            return 1;
        }
        *food = nf;
    }

    redraw_full_game(board, snake, *food, obs, score, theme);
    if (paused) show_pause_overlay(board);
    return 0;
}

int main(void) {
    int seed = 0;
    int mode;
    int theme_idx;
    const Theme *theme;
    int running = 1;
    Board *board;
    Snake *snake;
    Food *food;
    Score *score;
    Obstacles obstacles;
    Stats stats;
    int term_w, term_h;
    int board_w, board_h;
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

    /* init systems */
    memory_init();
    keyboard_init();
    terminal_install_winch_handler();
    screen_hide_cursor();

    /* load lifetime stats once */
    stats_load(&stats);

    while (running) {
        /* title screen — also seeds PRNG */
        mode = show_title_screen(&seed);

        /* theme selection */
        theme_idx = show_theme_screen();
        theme = theme_get(theme_idx);

        /* compute board from terminal size */
        term_w = get_terminal_width();
        term_h = get_terminal_height();

        if (term_w < 22 || term_h < 14) {
            keyboard_restore();
            screen_show_cursor();
            screen_clear();
            printf("Terminal too small! Need at least 22x14, got %dx%d\n",
                   term_w, term_h);
            return 1;
        }

        /* board is terminal minus borders and HUD space */
        board_w = term_w - 4;   /* 2 for border + 2 margin */
        board_h = term_h - 6;   /* 2 for border + 2 HUD lines + 2 margin */

        /* create game objects */
        board = board_create(board_w, board_h, mode);
        snake = snake_create(my_divide(board_w, 2), my_divide(board_h, 2));
        score = score_create();
        score_load_high(score);
        obstacles_init(&obstacles);
        food = food_spawn(snake, &obstacles, board_w, board_h, &seed);

        /* initial draw */
        screen_clear();
        render_border(board, theme);
        render_snake(snake, theme);
        if (food) render_food(food, board, theme);
        render_hud(score, board);
        screen_flush();

        paused = 0;
        last_score = 0;
        prev_level = 0;
        slow_ticks = 0;

        /* game loop */
        while (snake->alive) {
            if (terminal_consume_winch()) {
                int ar = apply_terminal_resize(&board_w, &board_h, board, snake, &food,
                                              &obstacles, score, theme, &seed, paused);
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
                    redraw_full_game(board, snake, food, &obstacles, score, theme);
                }
                continue;
            }
            if (paused) {
                usleep(50000);
                continue;
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

            /* 5. food check (before move) */
            grew = (food && nx == food->x && ny == food->y);

            /* save old tail for erasing */
            old_tail_x = snake->tail->x;
            old_tail_y = snake->tail->y;

            /* 6. execute move */
            snake_move(snake, nx, ny, grew);

            /* 7. post-move updates */
            if (grew) {
                /* determine points from food type */
                points = (food->type == FOOD_BONUS) ? 3 : 1;

                /* apply slow effect if slow food */
                if (food->type == FOOD_SLOW) {
                    slow_ticks = 30;
                }

                score_increment(score, points);
                stats_on_food(&stats, snake->length);

                /* check for level-up and spawn obstacles */
                if (score->level > prev_level) {
                    stats_on_level(&stats, score->level);
                    obstacles_spawn(&obstacles, snake, food,
                                   board_w, board_h, &seed);
                    render_obstacles(&obstacles, board, theme);
                    prev_level = score->level;
                }

                food_free(food);
                food = food_spawn(snake, &obstacles, board_w, board_h, &seed);
                if (food == NULL) {
                    /* victory! board is full */
                    show_victory(score, board);
                    score_save_high(score);
                    stats_on_game_over(&stats);
                    stats_save(&stats);
                    usleep(3000000);
                    break;
                }
                render_food(food, board, theme);
            } else {
                render_erase_tail(old_tail_x, old_tail_y, board);
            }

            /* 8. food despawn countdown */
            if (food && food->ticks_remaining > 0) {
                food->ticks_remaining--;
                if (food->ticks_remaining == 0) {
                    /* erase old food and spawn new */
                    screen_put_char(OFFSET_X + food->x, OFFSET_Y + food->y, ' ');
                    food_free(food);
                    food = food_spawn(snake, &obstacles, board_w, board_h, &seed);
                    if (food) render_food(food, board, theme);
                }
            }

            /* 9. render and wait */
            render_snake(snake, theme);
            if (score->score != last_score) {
                render_hud(score, board);
                last_score = score->score;
            }
            screen_flush();

            /* apply speed with optional slow effect */
            speed = score_get_speed(score);
            if (slow_ticks > 0) {
                speed += 50;
                slow_ticks--;
            }
            usleep(my_multiply(speed, 1000));
        }

        /* game over sequence (only if not quitting) */
        if (running && !snake->alive) {
            stats_on_game_over(&stats);
            stats_save(&stats);
            show_game_over(snake, board, score);
            score_save_high(score);

            key = show_game_over_prompt(board, score, &stats);
            if (key == 'q') {
                running = 0;
            }
        }

        /* save stats on quit (not just game over) */
        if (!running) {
            stats_save(&stats);
        }

        /* cleanup for restart or exit */
        snake_free(snake);
        if (food) food_free(food);
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
