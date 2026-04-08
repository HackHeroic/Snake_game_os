#ifndef MY_MATH_H
#define MY_MATH_H

typedef enum { DIR_UP = 0, DIR_RIGHT = 1, DIR_DOWN = 2, DIR_LEFT = 3 } Direction;

int my_multiply(int a, int b);
int my_divide(int a, int b);
int my_mod(int a, int b);
int my_abs(int a);
int my_min(int a, int b);
int my_max(int a, int b);
int my_clamp(int val, int min, int max);
int pseudo_random(int *seed);

#endif
