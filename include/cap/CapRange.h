/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <arch/Types.h>
#include <util/Math.h>
#include <Syscalls.h>

namespace nre {

class CapRange {
public:
	static const uintptr_t NO_HOTSPOT	= -1;

	explicit CapRange() : _start(), _count(), _attr(), _hotspot() {
	}
	explicit CapRange(uintptr_t start,size_t count,uint attr,uintptr_t hotspot = NO_HOTSPOT)
		: _start(start), _count(count), _attr(attr), _hotspot(hotspot) {
	}

	void revoke(bool self) {
		uintptr_t start = _start;
		size_t count = _count;
		while(count > 0) {
			uint minshift = Math::minshift(start,count);
			Syscalls::revoke(Crd(start,minshift,_attr),self);
			start += 1 << minshift;
			count -= 1 << minshift;
		}
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

static inline OStream &operator<<(OStream &os,const CapRange &cr) {
	os.writef("CapRange[start=%#x, count=%#x, hotspot=%#x, attr=%#x]",
			cr.start(),cr.count(),cr.hotspot(),cr.attr());
	return os;
}

}
