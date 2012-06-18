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
#include <mem/DataSpaceDesc.h>
#include <stream/OStream.h>
#include <Exception.h>

namespace nul {

class DataSpaceException : public Exception {
public:
	explicit DataSpaceException(ErrorCode code) throw() : Exception(code) {
	}
};

class Session;
class UtcbFrameRef;
template<class DS>
class DataSpaceManager;

class DataSpace {
	template<class DS>
	friend class DataSpaceManager;

public:
	enum RequestType {
		CREATE,
		JOIN,
		SHARE,
		DESTROY
	};

	static void create(DataSpaceDesc &desc,capsel_t *sel = 0,capsel_t *unmapsel = 0);

	DataSpace(size_t size,DataSpaceDesc::Type type,uint perm,uintptr_t phys = 0,uintptr_t virt = 0)
		: _desc(size,type,perm,phys,virt), _sel(ObjCap::INVALID), _unmapsel(ObjCap::INVALID) {
		create();
	}
	DataSpace(const DataSpaceDesc &desc) : _desc(desc), _sel(ObjCap::INVALID),
			_unmapsel(ObjCap::INVALID) {
		create();
	}
	DataSpace(const DataSpaceDesc &desc,capsel_t sel) : _desc(desc), _sel(sel),
			_unmapsel(ObjCap::INVALID) {
		join();
	}
	~DataSpace() {
		destroy();
	}

	capsel_t sel() const {
		return _sel;
	}
	capsel_t unmapsel() const {
		return _unmapsel;
	}
	const DataSpaceDesc &desc() const {
		return _desc;
	}
	uintptr_t virt() const {
		return _desc.virt();
	}
	size_t size() const {
		return _desc.size();
	}
	uint perm() const {
		return _desc.perm();
	}
	DataSpaceDesc::Type type() const {
		return _desc.type();
	}

	void share(Session &c);

private:
	void create();
	void join();
	void destroy();
	static void handle_response(UtcbFrameRef& uf);

	DataSpace(const DataSpace&);
	DataSpace& operator=(const DataSpace&);

	DataSpaceDesc _desc;
	capsel_t _sel;
	capsel_t _unmapsel;
};

static inline OStream &operator<<(OStream &os,const DataSpace &ds) {
	os.writef("DataSpace[%p..%p (%zu)]: perm=%#x, type=%u, sel=%#x, umsel=%#x",
			ds.virt(),ds.virt() + ds.size() - 1,ds.size(),ds.perm(),ds.type(),ds.sel(),ds.unmapsel());
	return os;
}

}
