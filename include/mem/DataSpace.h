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

namespace nul {

class DataSpace {
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

	DataSpace(size_t size,Type type,Perm perm,uintptr_t addr = 0)
		: _addr(addr), _size(size), _perm(perm), _type(type) {
	}
	virtual ~DataSpace() {
	}

	uintptr_t addr() const {
		return _addr;
	}
	size_t size() const {
		return _size;
	}
	Perm perm() const {
		return _perm;
	}

	virtual void map() {
		UtcbFrame uf;
		uf << _addr << _size << _perm << _type;
		CPU::current().map_pt->call(uf);
		uf >> _addr;
	}
	virtual void unmap() {
		// TODO
	}

private:
	DataSpace(const DataSpace&);
	DataSpace& operator=(const DataSpace&);

	uintptr_t _addr;
	size_t _size;
	Perm _perm;
	Type _type;
};

}
