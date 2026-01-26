#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>

#define KEY_ESCAPE (27)
#define KEY_CR (13)
#define KEY_LF (10)
#define IS_ENTER(c) ((c) == KEY_CR || (c) == KEY_LF)

void get_terminal_size(int *width, int *height);
int get_keypress(void);
void clear_screen(void);
// Check if the program was launched by double-clicking (Windows only)
bool is_double_clicked(void);
void ensure_terminal_size(int req_w, int req_h);

#endif // TERMINAL_H
