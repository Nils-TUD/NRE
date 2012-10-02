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
#include <Exception.h>
#include <util/Math.h>

namespace nre {

class RegionManagerException : public Exception {
public:
	DEFINE_EXCONSTRS(RegionManagerException)
};

class OStream;
class RegionManager;
OStream &operator<<(OStream &os,const RegionManager &rm);

class RegionManager {
	friend OStream &operator<<(OStream &os,const RegionManager &rm);

	enum {
		MAX_REGIONS		= 64
	};

	struct Region {
		uintptr_t addr;
		size_t size;
	};

public:
	typedef const Region *iterator;

	explicit RegionManager() : _regs() {
	}

	iterator begin() const {
		return _regs + 0;
	}
	iterator end() const {
		return _regs + MAX_REGIONS;
	}

	size_t total_size() const {
		size_t s = 0;
		for(iterator it = begin(); it != end(); ++it)
			s += it->size;
		return s;
	}

	uintptr_t alloc(size_t size,uint align = 1) {
		Region *r = get(size,align);
		if(!r) {
			throw RegionManagerException(E_CAPACITY,64,
					"Unable to allocate %zu bytes aligned to %u",size,align);
		}
		uintptr_t org = r->addr;
		uintptr_t start = (r->addr + align - 1) & ~(align - 1);
		size += start - r->addr;
		r->addr += size;
		r->size -= size;
		// if there is space left before the start due to aligning, free it
		if(start > org) {
			// do we need a new region?
			if(r->size > 0)
				r = get_free();
			r->addr = org;
			r->size = start - org;
		}
		return start;
	}

	void alloc_region(uintptr_t addr,size_t size) {
		Region *r = get(addr,size,true);
		if(!r)
			throw RegionManagerException(E_EXISTS,64,"Region %p..%p not found",addr,addr + size);
		remove_from(r,addr,size);
	}

	void remove(uintptr_t addr,size_t size) {
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size && Math::overlapped(addr,size,_regs[i].addr,_regs[i].size))
				remove_from(_regs + i,addr,size);
		}
	}

	void free(uintptr_t addr,size_t size) {
		Region *p = 0,*n = 0;
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size > 0) {
				if(_regs[i].addr + _regs[i].size == addr)
					p = _regs + i;
				else if(_regs[i].addr == addr + size)
					n = _regs + i;
				if(n && p)
					break;
			}
		}

		if(n && p) {
			p->size += size + n->size;
			n->size = 0;
		}
		else if(n) {
			n->addr -= size;
			n->size += size;
		}
		else if(p)
			p->size += size;
		else {
			Region *f = get_free();
			f->addr = addr;
			f->size = size;
		}
	}

private:
	RegionManager(const RegionManager&);
	RegionManager& operator=(const RegionManager&);

	Region *get(size_t size,uint align) {
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size >= size) {
				uintptr_t start = (_regs[i].addr + align - 1) & ~(align - 1);
				if(_regs[i].size - (start - _regs[i].addr) >= size)
					return _regs + i;
			}
		}
		return 0;
	}
	Region *get(uintptr_t addr,size_t size,bool) {
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(addr >= _regs[i].addr && _regs[i].addr + _regs[i].size >= addr + size)
				return _regs + i;
		}
		return 0;
	}
	Region *get_free() {
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size == 0)
				return _regs + i;
		}
		throw RegionManagerException(E_CAPACITY,"No free regions");
	}

	void remove_from(Region *r,uintptr_t addr,size_t size) {
		// complete region should be removed?
		if(addr <= r->addr && addr + size >= r->addr + r->size)
			r->size = 0;
		// at the beginning?
		else if(addr <= r->addr) {
			r->size -= (addr + size) - r->addr;
			r->addr = addr + size;
		}
		// at the end?
		else if(addr + size >= r->addr + r->size)
			r->size = addr - r->addr;
		// in the middle
		else {
			Region *nr = get_free();
			nr->addr = addr + size;
			nr->size = (r->addr + r->size) - nr->addr;
			r->size = addr - r->addr;
		}
	}

	Region _regs[MAX_REGIONS];
};

}
