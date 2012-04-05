/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <pd/CapSpace.h>
#include <pd/ResourceSpace.h>

class Pd {
public:
	static Pd *current();

	Pd(cap_t cap) : _io(0,0), _mem(0,0), _obj(), _cap(cap) {
	}

	cap_t cap() const {
		return _cap;
	}
	ResourceSpace &io() {
		return _io;
	}
	ResourceSpace &mem() {
		return _mem;
	}
	CapSpace &obj() {
		return _obj;
	}

private:
	ResourceSpace _io;
	ResourceSpace _mem;
	CapSpace _obj;
	cap_t _cap;
};
