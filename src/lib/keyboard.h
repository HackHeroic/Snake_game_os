#ifndef MY_KEYBOARD_H
#define MY_KEYBOARD_H

#define KEY_NONE 0
#define KEY_UP 1000
#define KEY_DOWN 1001
#define KEY_LEFT 1002
#define KEY_RIGHT 1003

void keyboard_init(void);
void keyboard_restore(void);
int key_pressed(void);
int read_key(void);
/* Returns 1 once after SIGWINCH; safe to call each frame. */
int keyboard_consume_resize(void);

#endif
