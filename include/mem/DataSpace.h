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
#include <kobj/ObjCap.h>
#include <stream/OStream.h>
#include <Exception.h>

namespace nul {

class DataSpaceException : public Exception {
public:
	explicit DataSpaceException(ErrorCode code) throw() : Exception(code) {
	}
};

class Session;
class DataSpace;
class UtcbFrameRef;
class DataSpaceManager;

UtcbFrameRef &operator >>(UtcbFrameRef &uf,DataSpace &ds);

class DataSpace {
	friend UtcbFrameRef &operator >>(UtcbFrameRef &uf,DataSpace &ds);
	friend class DataSpaceManager;

	enum RequestType {
		CREATE,
		JOIN,
		SHARE,
		DESTROY
	};

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

	DataSpace() : _virt(), _phys(), _size(), _perm(), _type(), _sel(ObjCap::INVALID),
			_unmapsel(ObjCap::INVALID) {
	}
	DataSpace(size_t size,Type type,uint perm,uintptr_t phys = 0,uintptr_t virt = 0)
		: _virt(virt), _phys(phys), _size(size), _perm(perm), _type(type), _sel(ObjCap::INVALID),
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

	void map() {
		if(_sel == ObjCap::INVALID)
			create();
		else
			join();
	}
	void share(Session &c);
	void unmap();

private:
	void create();
	void join();
	void handle_response(UtcbFrameRef& uf);

	uintptr_t _virt;
	uintptr_t _phys;
	size_t _size;
	uint _perm;
	Type _type;
	capsel_t _sel;
	capsel_t _unmapsel;
};

static inline OStream &operator<<(OStream &os,const DataSpace &ds) {
	os.writef("DataSpace[%p..%p (%zu)]: perm=%#x, type=%u, sel=%#x, umsel=%#x",
			ds.virt(),ds.virt() + ds.size() - 1,ds.size(),ds.perm(),ds.type(),ds.sel(),ds.unmapsel());
	return os;
}

}
