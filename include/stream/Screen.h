/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <stream/OStream.h>
#include <cstring>

namespace nul {

class Screen : public OStream {
	enum {
		COLS		= 80,
		ROWS		= 25,
		TAB_WIDTH	= 4
	};
	enum {
		BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE
	};

public:
	static Screen& get() {
		return _inst;
	}

	virtual void write(char c);
	void clear() {
		//memset(SCREEN,0,ROWS * COLS * 2);
		_col = _row = 0;
	}

private:
	Screen() : OStream() {
	}
	void move();

	int _col;
	int _row;
	static Screen _inst;
	static char* const SCREEN;
	static const char COLOR = BLACK << 4 | WHITE;
};

}
