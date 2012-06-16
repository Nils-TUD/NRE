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
#include <Exception.h>
#include <stream/OStream.h>

namespace nul {

class RegionManagerException : public Exception {
public:
	explicit RegionManagerException(ErrorCode code) throw() : Exception(code) {
	}
};

class RegionManager {
	enum {
		MAX_REGIONS		= 64
	};

	struct Region {
		uintptr_t addr;
		size_t size;
	};

public:
	typedef const Region *iterator;

	RegionManager() : _regs() {
	}

	iterator begin() const {
		return _regs + 0;
	}
	iterator end() const {
		return _regs + MAX_REGIONS;
	}

	uintptr_t alloc(size_t size) {
		Region *r = get(size);
		if(!r)
			throw RegionManagerException(E_CAPACITY);
		r->addr += size;
		r->size -= size;
		return r->addr - size;
	}

	void alloc(uintptr_t addr,size_t size) {
		Region *r = get(addr,size);
		if(!r)
			throw RegionManagerException(E_EXISTS);
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

	void write(OStream &os) const {
		os.writef("Regions:\n");
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size > 0) {
				os.writef(
					"\t%zu: %p .. %p (%zu)\n",i,_regs[i].addr,
						_regs[i].addr + _regs[i].size,_regs[i].size
				);
			}
		}
	}

private:
	RegionManager(const RegionManager&);
	RegionManager& operator=(const RegionManager&);

	Region *get(size_t size) {
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size >= size)
				return _regs + i;
		}
		return 0;
	}
	Region *get(uintptr_t addr,size_t size) {
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
		throw RegionManagerException(E_CAPACITY);
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
