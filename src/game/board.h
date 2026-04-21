#ifndef BOARD_H
#define BOARD_H

typedef struct {
    int width, height;
    int mode;  /* 0 = Classic, 1 = Wrap */
} Board;

Board *board_create(int width, int height, int mode);
int board_check_wall_collision(Board *b, int x, int y);
void board_wrap_position(Board *b, int *x, int *y);
void board_free(Board *b);
int get_terminal_width(void);
int get_terminal_height(void);

void terminal_install_winch_handler(void);
int terminal_consume_winch(void);

void board_set_size(Board *b, int width, int height);

#endif
