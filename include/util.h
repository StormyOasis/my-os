#ifndef __UTIL_H_
#define __UTIL_H_

#include "../include/defs.h"
#include "../include/kernel.h"

#define BLACK   0
#define BLUE    1
#define GREEN   2
#define CYAN    3
#define RED     4
#define MAGENTA 5
#define AMBER   6
#define GRAY    7
#define BRIGHT  8

#define ATTRIB(bg,fg) ((fg)|((bg)<<4))

void printf(const char *, ...);
void debug_printf(const char *, ...);
void dprintf(const char *, ...);

void putchar(char);
void debug_putchar(char);

UINT strlen(const char*);
char *strcpy(char *dest, const char *src);
char * strcat(const char * str1, const char * str2);

void itoa (char *buf, int base, int d);
void ultoa (char *buf, int base, ULONG d);

ULONG pow(ULONG, ULONG);

void wait(int);

static void Scroll(TaskData *);

void do_panic(const char *, const char * func, const char * file, int line);

void setSystemX(ULONG x);
void clrscr();

void halt();

void DisableInterrupts();
void EnableInterrupts();

void inline SetBit(void *t, UINT loc);
void inline ClearBit(void *t, UINT loc);
char inline GetBitState(void *t, UINT loc);

#define Sleep(x) wait(x);
#define panic(str) do_panic(str, __FUNCTION__, __FILE__, __LINE__)

#endif
