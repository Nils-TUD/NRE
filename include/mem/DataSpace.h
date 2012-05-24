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
#include <utcb/UtcbFrame.h>
#include <CPU.h>

namespace nul {

class DataSpace {
	friend class DataSpaceManager;
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

	DataSpace() : _virt(), _origin(), _size(), _perm(), _type(), _own(false), _sel(ObjCap::INVALID) {
	}
	DataSpace(size_t size,Type type,Perm perm,uintptr_t origin = 0)
		: _virt(), _origin(origin), _size(size), _perm(perm), _type(type), _own(true), _sel() {
		CapHolder cap;
		Syscalls::create_sm(cap.get(),0,Pd::current()->sel());
		_sel = cap.release();
	}

	capsel_t sel() const {
		return _sel;
	}
	uintptr_t virt() const {
		return _virt;
	}
	uintptr_t origin() const {
		return _origin;
	}
	size_t size() const {
		return _size;
	}
	Perm perm() const {
		return _perm;
	}
	Type type() const {
		return _type;
	}

	void map() {
		_virt = domap();
	}
	void unmap() {
		// TODO
	}

private:
	uintptr_t domap() {
		UtcbFrame uf;
		if(_own)
			uf.delegate(_sel);
		else
			uf.translate(_sel);
		uf << _virt << _origin << _size << _perm << _type;
		CPU::current().map_pt->call(uf);
		// TODO error-handling
		uintptr_t res;
		uf >> res;
		return res;
	}
	void dounmap() {
		// TODO
	}

	uintptr_t _virt;
	uintptr_t _origin;
	size_t _size;
	Perm _perm;
	Type _type;
	bool _own;
	capsel_t _sel;
};

UtcbFrameRef &operator >>(UtcbFrameRef &uf,DataSpace &ds) {
	uf >> ds._virt >> ds._origin >> ds._size >> ds._type >> ds._perm;
	TypedItem ti;
	uf.get_typed(ti);
	ds._sel = ti.crd().cap();
	ds._own = false;
	return uf;
}

}
