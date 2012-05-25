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

#include <kobj/Sm.h>
#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>

namespace nul {

class DataSpace;
static UtcbFrameRef &operator >>(UtcbFrameRef &uf,DataSpace &ds);

class DataSpace {
	friend UtcbFrameRef &operator >>(UtcbFrameRef &uf,DataSpace &ds);

public:
	enum Type {
		ANONYMOUS,
		LOCKED
	};
	enum Perm {
		// note that this equals the values in a Crd
		R	= 1 << 0,
		W	= 1 << 1,
		X	= 1 << 2,
		RW	= R | W,
		RX	= R | X,
		RWX	= R | W | X,
	};

	DataSpace() : _virt(), _phys(), _size(), _perm(), _type(), _own(true), _sel(ObjCap::INVALID),
			_unmapsel(ObjCap::INVALID) {
	}
	DataSpace(size_t size,Type type,uint perm,uintptr_t phys = 0)
		: _virt(), _phys(phys), _size(size), _perm(perm), _type(type), _own(true), _sel(ObjCap::INVALID),
		  _unmapsel(ObjCap::INVALID) {
	}
	~DataSpace() {
	}

	capsel_t sel() const {
		return _sel;
	}
	capsel_t unmapsel() const {
		return _unmapsel;
	}
	uintptr_t virt() const {
		return _virt;
	}
	uintptr_t phys() const {
		return _phys;
	}
	size_t size() const {
		return _size;
	}
	uint perm() const {
		return _perm;
	}
	Type type() const {
		return _type;
	}

	void map();
	void unmap();

private:
	uintptr_t _virt;
	uintptr_t _phys;
	size_t _size;
	uint _perm;
	Type _type;
	bool _own;
	capsel_t _sel;
	capsel_t _unmapsel;
};

static inline UtcbFrameRef &operator >>(UtcbFrameRef &uf,DataSpace &ds) {
	int type;
	// TODO check for errors
	uf >> type >> ds._virt >> ds._phys >> ds._size >> ds._perm >> ds._type;
	TypedItem ti;
	uf.get_typed(ti);
	if(type == 0)
		ds._sel = ti.crd().cap();
	else
		ds._unmapsel = ti.crd().cap();
	return uf;
}

}
