/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <Types.h>

class CapSpace {
public:
	explicit CapSpace(cap_t off = 0) : _off(off) {
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
