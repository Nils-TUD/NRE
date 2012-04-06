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

#include <Types.h>
#include <Hip.h>

class CapSpace {
public:
	explicit CapSpace(cap_t off = Hip::get().object_caps()) : _off(off) {
	}
	cap_t allocate(unsigned = 1) {
		return _off++;
	}
	void free(cap_t,unsigned = 1) {
		// TODO implement me
	}

private:
	CapSpace(const CapSpace&);
	CapSpace& operator=(const CapSpace&);

private:
	cap_t _off;
};
