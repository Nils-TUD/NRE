/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <cap/CapSpace.h>

class CapHolder {
public:
	explicit CapHolder(CapSpace& space)
		: _space(space), _cap(_space.allocate()), _owned(true) {
	}
	~CapHolder() {
		if(_owned)
			_space.free(_cap);
	}
	cap_t get() const {
		return _cap;
	}
	cap_t release() {
		_owned = false;
		return _cap;
	}

private:
	CapHolder(const CapHolder&);
	CapHolder& operator=(const CapHolder&);

private:
	CapSpace &_space;
	cap_t _cap;
	bool _owned;
};
