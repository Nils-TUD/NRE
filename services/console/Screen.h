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
#include <arch/ExecEnv.h>
#include <services/Console.h>

class Screen {
public:
	enum {
		COLS		= nre::Console::COLS,
		ROWS		= nre::Console::ROWS,
		SIZE		= COLS * ROWS * 2,
		PAGES		= nre::Console::PAGES,
		PAGE_SIZE	= nre::Console::PAGE_SIZE,
		TEXT_OFF	= nre::Console::TEXT_OFF,
		TEXT_PAGES	= nre::Console::TEXT_PAGES,
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

	virtual nre::DataSpace &mem() = 0;
	virtual void set_regs(const nre::Console::Register &regs) = 0;

private:
	uint _current;
};
