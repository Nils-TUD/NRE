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
