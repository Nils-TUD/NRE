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

#include <kobj/Pt.h>
#include <kobj/UserSm.h>
#include <mem/RegionManager.h>
#include <mem/DataSpaceManager.h>

class PhysicalMemory {
public:
	class RootDataSpace;
	friend class RootDataSpace;

	class RootDataSpace {
	public:
		RootDataSpace() : _desc(), _sel(), _unmapsel(), _next() {
		}
		RootDataSpace(const nre::DataSpaceDesc &desc);
		RootDataSpace(capsel_t);
		~RootDataSpace();

		capsel_t sel() const {
			return _sel;
		}
		capsel_t unmapsel() const {
			return _unmapsel;
		}
		const nre::DataSpaceDesc &desc() const {
			return _desc;
		}

		// we have to provide custom new and delete operators since we can't use dynamic memory for
		// building dynamic memory :)
		static void *operator new(size_t size) throw();
		static void operator delete(void *ptr) throw();

	private:
		static void revoke_mem(uintptr_t addr,size_t size,bool self = false);

		nre::DataSpaceDesc _desc;
		capsel_t _sel;
		capsel_t _unmapsel;
		RootDataSpace *_next;
		static RootDataSpace *_free;
	};

	static uintptr_t alloc(size_t size) {
		return _mem.alloc(size);
	}
	static void free(uintptr_t phys,size_t size) {
		_mem.free(phys,size);
	}

	static void add(uintptr_t addr,size_t size);
	static void remove(uintptr_t addr,size_t size);
	static void map_all();

	static size_t total_size() {
		return _totalsize;
	}
	static size_t free_size() {
		return _mem.total_size();
	}

	static const nre::RegionManager &regions() {
		return _mem;
	}

	PORTAL static void portal_dataspace(capsel_t);

private:
	static bool can_map(uintptr_t phys,size_t size,uint &flags);

	PhysicalMemory();

	static size_t _totalsize;
	static nre::RegionManager _mem;
	static nre::DataSpaceManager<RootDataSpace> _dsmng;
};

static inline nre::OStream &operator<<(nre::OStream &os,const PhysicalMemory::RootDataSpace &ds) {
	os.writef("RootDataSpace[sel=%#x, umsel=%#x]: ",ds.sel(),ds.unmapsel());
	os << ds.desc();
	return os;
}
