/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <arch/Types.h>
#include <arch/ExecEnv.h>
#include <services/Console.h>

class Screen {
public:
	static const uint COLS          = nre::Console::COLS;
	static const uint ROWS          = nre::Console::ROWS;
	static const size_t SIZE        = COLS * ROWS * 2;
	static const size_t PAGES       = nre::Console::PAGES;
	static const size_t PAGE_SIZE   = nre::Console::PAGE_SIZE;
	static const size_t TEXT_OFF    = nre::Console::TEXT_OFF;
	static const size_t TEXT_PAGES  = nre::Console::TEXT_PAGES;

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
