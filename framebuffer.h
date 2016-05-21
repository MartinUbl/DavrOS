#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

#define FRAMEBUFFER_WIDTH 80
#define FRAMEBUFFER_HEIGHT 25

// puts character to output
void putchar(char c);
// writes zero-terminated string
void echo(const char* str);
// clears screen and resets cursor position to 0;0
void clearscreen();

#endif
