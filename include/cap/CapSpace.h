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
	static CapSpace& get() {
		return _inst;
	}

private:
	explicit CapSpace(cap_t off = Hip::get().object_caps()) : _off(off) {
	}

public:
	cap_t allocate(unsigned count = 1) {
		cap_t res = _off;
		if(res + count < res || res + count > Hip::get().cfg_cap)
			throw Exception("Out of caps");
		_off += count;
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
