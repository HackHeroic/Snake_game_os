#include "lib/memory.h"
#include "lib/screen.h"
#include "lib/keyboard.h"
#include "lib/math.h"
#include "game/snake.h"
#include "game/food.h"
#include "game/board.h"
#include "game/score.h"
#include "ui/renderer.h"
#include "ui/screens.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    int seed = 0;
    int mode;
    int running = 1;
    Board *board;
    Snake *snake;
    Food *food;
    Score *score;
    int term_w, term_h;
    int board_w, board_h;
    int paused;
    int key;
    int nx, ny;
    int grew;
    int old_tail_x, old_tail_y;
    int last_score;

    /* init systems */
    memory_init();
    keyboard_init();
    screen_hide_cursor();

    while (running) {
        /* title screen — also seeds PRNG */
        mode = show_title_screen(&seed);

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
        food = food_spawn(snake, board_w, board_h, &seed);

        /* initial draw */
        screen_clear();
        render_border(board);
        render_snake(snake);
        if (food) render_food(food, board);
        render_hud(score, board);
        screen_flush();

        paused = 0;
        last_score = 0;

        /* game loop */
        while (snake->alive) {
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
                    /* redraw to clear pause text */
                    screen_clear();
                    render_border(board);
                    render_snake(snake);
                    if (food) render_food(food, board);
                    render_hud(score, board);
                    screen_flush();
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

            /* 4. food check (before move) */
            grew = (food && nx == food->x && ny == food->y);

            /* save old tail for erasing */
            old_tail_x = snake->tail->x;
            old_tail_y = snake->tail->y;

            /* 5. execute move */
            snake_move(snake, nx, ny, grew);

            /* 6. post-move updates */
            if (grew) {
                score_increment(score, 1);
                food_free(food);
                food = food_spawn(snake, board_w, board_h, &seed);
                if (food == NULL) {
                    /* victory! board is full */
                    show_victory(score, board);
                    score_save_high(score);
                    usleep(3000000);
                    break;
                }
                render_food(food, board);
            } else {
                render_erase_tail(old_tail_x, old_tail_y, board);
            }

            /* 7. render and wait */
            render_snake(snake);
            if (score->score != last_score) {
                render_hud(score, board);
                last_score = score->score;
            }
            screen_flush();

            usleep(my_multiply(score_get_speed(score), 1000));
        }

        /* game over sequence (only if not quitting) */
        if (running && !snake->alive) {
            show_game_over(snake, board, score);
            score_save_high(score);

            key = show_game_over_prompt(board, score);
            if (key == 'q') {
                running = 0;
            }
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
