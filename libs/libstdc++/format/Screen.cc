/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <format/Screen.h>

namespace nul {

char* const Screen::SCREEN = (char* const)0xB9000;

void Screen::printc(char c) {
	if(_col >= COLS) {
		_row++;
		_col = 0;
	}
	move();

	char *video = SCREEN + _row * COLS * 2 + _col * 2;
	if(c == '\n') {
		_row++;
		_col = 0;
	}
	else if(c == '\r')
		_col = 0;
	else if(c == '\t') {
		unsigned i = TAB_WIDTH - _col % TAB_WIDTH;
		while(i-- > 0)
			printc(' ');
	}
	else {
		*video = c;
		video++;
		*video = COLOR;

		_col++;
	}
}

void Screen::move() {
	if(_row >= ROWS) {
		memmove(SCREEN,SCREEN + COLS * 2,(ROWS - 1) * COLS * 2);
		_row--;
	}
}

}
