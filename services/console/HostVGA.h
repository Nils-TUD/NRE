/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2009, Bernhard Kauer <bk@vmmon.org>
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

#include <arch/ExecEnv.h>
#include <kobj/Ports.h>
#include <mem/DataSpace.h>
#include <services/Console.h>

#include "ConsoleSessionData.h"
#include "Screen.h"

class HostVGA : public Screen {
	enum Register {
		CURSOR_HI		= 0xa,
		CURSOR_LO		= 0xb,
		START_ADDR_HI	= 0xc,
		START_ADDR_LO	= 0xd,
		CURSOR_LOC_HI	= 0xe,
		CURSOR_LOC_LO	= 0xf
	};
	enum {
		VGA_MEM			= 0xa0000,
		VGA_PAGE_SIZE	= nre::ExecEnv::PAGE_SIZE,
	};

public:
	explicit HostVGA() : Screen(), _ports(0x3d4,2),
		_ds(VGA_PAGE_SIZE * PAGES,nre::DataSpaceDesc::ANONYMOUS,nre::DataSpaceDesc::RW,VGA_MEM) {
	}

	virtual nre::DataSpace &mem() {
		return _ds;
	}
	virtual void set_regs(const nre::Console::Register &regs);

private:
	void write(Register reg,uint8_t val) {
		_ports.out<uint8_t>(reg,0);
		_ports.out<uint8_t>(val,1);
	}

	nre::Ports _ports;
	nre::DataSpace _ds;
	nre::Console::Register _last;
};
