/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <format/Format.h>
#include <cstring>

namespace nul {

class Screen : public Format {
	enum {
		COLS		= 80,
		ROWS		= 25,
		TAB_WIDTH	= 4
	};
	enum {
		BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE
	};

	static char* const SCREEN;
	static const char COLOR = BLACK << 4 | WHITE;

public:
	explicit Screen() : _col(), _row() {
		memset(SCREEN,0,ROWS * COLS * 2);
	}
	virtual ~Screen() {
	}

protected:
	virtual void printc(char c);

private:
	void move();

	int _col;
	int _row;
};

}
