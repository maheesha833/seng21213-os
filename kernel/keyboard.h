#ifndef KEYBOARD_H
#define KEYBOARD_H
#include "../include/types.h"
#define KB_BUF_SIZE 256
#define KEY_UP    0x11
#define KEY_DOWN  0x12
void kb_init(void);
char kb_getchar(void);
int  kb_readline(char *buf, int len);
#endif
