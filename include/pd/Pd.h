/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <ec/Ec.h>

class Pd {
public:
	static Pd *current() {
		return Ec::current()->pd();
	}

	cap_t cap() const {
		return _cap;
	}
	CapSpace &io() {
		return _io;
	}
	CapSpace &mem() {
		return _mem;
	}
	CapSpace &obj() {
		return _obj;
	}

private:
	CapSpace _io;
	CapSpace _mem;
	CapSpace _obj;
	cap_t _cap;
};
