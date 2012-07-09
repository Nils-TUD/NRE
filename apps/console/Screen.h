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
#include <dev/Console.h>

class Screen {
public:
	enum {
		COLS		= nul::Console::COLS,
		ROWS		= nul::Console::ROWS,
		SIZE		= COLS * ROWS * 2,
		PAGES		= nul::Console::PAGES,
		PAGE_SIZE	= nul::Console::PAGE_SIZE,
		TEXT_OFF	= nul::Console::TEXT_OFF,
		TEXT_PAGES	= nul::Console::TEXT_PAGES,
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

	virtual nul::DataSpace &mem() = 0;
	virtual void set_regs(const nul::Console::Register &regs) = 0;

private:
	uint _current;
};
