#ifndef MY_SCREEN_H
#define MY_SCREEN_H

void screen_clear(void);
void screen_goto(int x, int y);
void screen_put_char(int x, int y, char c);
void screen_put_str(int x, int y, const char *s);
void screen_set_color(int color_code);
void screen_reset_color(void);
void screen_hide_cursor(void);
void screen_show_cursor(void);
void screen_flush(void);

#endif
