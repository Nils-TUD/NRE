/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <ex/Exception.h>
#include <Types.h>
#include <Hip.h>

namespace nul {

class CapSpace {
public:
	enum {
		EV_DIVIDE		= 0x0,
		EV_DEBUG		= 0x1,
		EV_BREAKPOINT	= 0x3,
		EV_OVERFLOW		= 0x4,
		EV_BOUNDRANGE	= 0x5,
		EV_UNDEFOP		= 0x6,
		EV_NOMATHPROC	= 0x7,
		EV_DBLFAULT		= 0x8,
		EV_TSS			= 0xA,
		EV_INVSEG		= 0xB,
		EV_STACK		= 0xC,
		EV_GENPROT		= 0xD,
		EV_PAGEFAULT	= 0xE,
		EV_MATHFAULT	= 0x10,
		EV_ALIGNCHK		= 0x11,
		EV_MACHCHK		= 0x12,
		EV_SIMD			= 0x13,
		EV_STARTUP		= 0x1E,
		EV_RECALL		= 0x1F,

		SRV_REGISTER	= 0x20,		// register service
		SRV_GET			= 0x21,		// get service
	};

	static CapSpace& get() {
		return _inst;
	}

private:
	explicit CapSpace(cap_t off = Hip::get().object_caps()) : _off(off) {
	}

public:
	cap_t allocate(unsigned count = 1,unsigned align = 1) {
		cap_t res = (_off + align - 1) & ~(align - 1);
		if(res + count < res || res + count > Hip::get().cfg_cap)
			throw Exception("Out of caps");
		_off = res + count;
		return res;
	}
	void free(cap_t,unsigned = 1) {
		// TODO implement me
	}

private:
	CapSpace(const CapSpace&);
	CapSpace& operator=(const CapSpace&);

	static CapSpace _inst;
	cap_t _off;
};

}
