#include "video.h"

char* const Video::SCREEN = (char* const)0xB8000;
int Video::col = 0;
int Video::row = 0;
int Video::color = BLACK << 4 | WHITE;
const char *Video::chars = "0123456789ABCDEF";

void Video::clear() {
	Util::set(SCREEN,0,ROWS * COLS * 2);
}

void Video::putc(char c) {
	if(col >= COLS) {
		row++;
		col = 0;
	}
	move();

	char *video = SCREEN + row * COLS * 2 + col * 2;
	if(c == '\n') {
		row++;
		col = 0;
	}
	else if(c == '\r')
		col = 0;
	else if(c == '\t') {
		unsigned i = TAB_WIDTH - col % TAB_WIDTH;
		while(i-- > 0)
			putc(' ');
	}
	else {
		*video = c;
		video++;
		*video = color; 

		col++;
	}
}

void Video::puts(const char* str) {
	while(*str)
		putc(*str++);
}

void Video::move() {
	if(row >= ROWS) {
		Util::move(SCREEN,SCREEN + COLS * 2,(ROWS - 1) * COLS * 2);
		Util::set(SCREEN + (ROWS - 1) * COLS * 2,0,COLS * 2);
		row--;
	}
}
