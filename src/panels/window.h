#ifndef WINDOW_H
#define WINDOW_H

#include <cstring>

#ifdef _WIN32
#include <ncurses/ncurses.h>
#else
#include <curses.h>
#endif

#include <iostream>
#include <string>
#include "../cpu.h"

#define SCREEN_WIDTH 43
#define SCREEN_HEIGHT 203

WINDOW *create_newPanel(int height, int width, int starty, int startx);

WINDOW *create_newPanel(int height, int width, int starty, int startx, const char *title);

void destroy_Panel(WINDOW *local_win);

void setTitle(WINDOW *local_win, const char *title);

void print(WINDOW *local_win, int y, int x, const char *text);

void print(WINDOW *local_win, int y, int x, const char *text, int num);

void print8(WINDOW *local_win, int y, int x, const char *text, int num);

void print16(WINDOW *local_win, int y, int x, const char *text, int num);

void printff(WINDOW *local_win, int y, int x, const char *text, float num);

void displayMemory(WINDOW *local_win, int mem_size, CPU &core);

void displayStack(WINDOW *local_win, int stack_size, CPU &core);

void printColor(WINDOW *local_win, int color, int y, int x, const char *text);

void get_user_input(WINDOW *terminal, char *input, size_t size);

#endif // WINDOW_H