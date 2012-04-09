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

namespace nul {

class Region {
public:
	enum Perm {
		R	= 1 << 0,
		W	= 1 << 1,
		X	= 1 << 2,
	};

public:
	Region(uintptr_t begin,size_t size,Perm perms)
		: _begin(begin), _size(size), _perms(perms) {
	}

	uintptr_t begin() const {
		return _begin;
	}
	uintptr_t end() const {
		return _begin + _size;
	}
	size_t size() const {
		return _size;
	}
	Perm perms() const {
		return _perms;
	}

private:
	uintptr_t _begin;
	size_t _size;
	Perm _perms;
};

}
