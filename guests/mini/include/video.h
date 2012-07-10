#pragma once

#include "util.h"
#include <stdarg.h>

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

	template<typename T>
	static void putn(T n) {
		if(n < 0) {
			putc('-');
			n = -n;
		}
		if(n >= 10)
			putn<T>(n / 10);
		putc('0' + (n % 10));
	}

	template<typename T>
	static void putu(T u,uint base) {
		if(u >= base)
			putu<T>(u / base,base);
		putc(chars[(u % base)]);
	}

	static void printf(const char *fmt,...) {
		va_list ap;
		va_start(ap,fmt);
		vprintf(fmt,ap);
		va_end(ap);
	}
	static void vprintf(const char *fmt,va_list ap);

private:
	static void move();

private:
	static int col;
	static int row;
	static int color;
	static const char *chars;
};

