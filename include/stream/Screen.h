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

class Log;

class Screen : public OStream {
	friend class Log;

	enum {
		COLS		= 80,
		ROWS		= 25,
		TAB_WIDTH	= 4
	};
	enum {
		BLACK,BLUE,GREEN,CYAN,RED,MARGENTA,ORANGE,WHITE,GRAY,LIGHTBLUE
	};

public:
	explicit Screen(char *const addr) : OStream(), _screen(addr), _col(), _row() {
	}

	void clear() {
		memset(_screen,0,ROWS * COLS * 2);
		_col = _row = 0;
	}

private:
	void move();
	virtual void write(char c);

	char *const _screen;
	int _col;
	int _row;
	static const char COLOR = BLACK << 4 | WHITE;
};

}
