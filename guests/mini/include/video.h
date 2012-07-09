#pragma once

#include "util.h"

class Video {
private:
	static char* const SCREEN;
	enum {
		COLS		= 80,
		ROWS		= 25,
		TAB_WIDTH	= 4
	};

public:
	enum {
		BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE
	};

public:
	static void clear();
	static inline void set_color(int fg,int bg) {
		color = ((bg & 0xF) << 4) | (fg & 0xF);
	}
	static void putc(char c);
	static void puts(const char* str);

private:
	static void move();

private:
	static int col;
	static int row;
	static int color;
};

