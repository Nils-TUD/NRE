/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <arch/Types.h>

class Screen {
public:
	enum {
		PAGE_COUNT	= 8,
		COLS		= 80,
		ROWS		= 25,
		SIZE		= COLS * ROWS * 2
	};

	explicit Screen() {
	}
	virtual ~Screen() {
	}

	uint current() const {
		return _current;
	}
	void current(uint current) {
		_current = current;
	}

	virtual void paint(uint uid,uint8_t x,uint8_t y,uint8_t *buffer,size_t count) = 0;
	virtual void set_page(uint uid,uint page) = 0;

private:
	uint _current;
};
