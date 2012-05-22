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

#include <arch/Types.h>

namespace nul {

class CapRange {
public:
	CapRange() : _start(), _count(), _attr(), _hotspot() {
	}
	CapRange(uintptr_t start,size_t count,uint attr,uintptr_t hotspot = 0)
		: _start(start), _count(count), _attr(attr), _hotspot(hotspot) {
	}

	uintptr_t start() const {
		return _start;
	}
	void start(uintptr_t start) {
		_start = start;
	}
	size_t count() const {
		return _count;
	}
	void count(size_t count) {
		_count = count;
	}
	uint attr() const {
		return _attr;
	}
	void attr(uint attr) {
		_attr = attr;
	}
	uintptr_t hotspot() const {
		return _hotspot;
	}
	void hotspot(uintptr_t hotspot) {
		_hotspot = hotspot;
	}

private:
	uintptr_t _start;
	size_t _count;
	uint _attr;
	uintptr_t _hotspot;
};

}
