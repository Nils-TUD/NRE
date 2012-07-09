/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <arch/Types.h>
#include <Exception.h>
#include <mem/DataSpaceDesc.h>
#include <util/Math.h>
#include <Assert.h>

extern void test_reglist();

namespace nul {

class ChildMemoryException : public Exception {
public:
	explicit ChildMemoryException(ErrorCode code) throw() : Exception(code) {
	}
};

class OStream;
class ChildMemory;
OStream &operator<<(OStream &os,const ChildMemory &cm);

class ChildMemory {
	friend void ::test_reglist();
	friend OStream &operator<<(OStream &os,const ChildMemory &cm);

	enum {
		MAX_REGIONS		= 512,
		MAX_DS			= 64
	};

	struct Region {
		uintptr_t src;
		uintptr_t begin;
		size_t size;
		uint flags;
	};

public:
	struct DS {
		DataSpaceDesc desc;
		capsel_t unmapsel;
	};

	typedef const DS *ds_iterator;

	enum Perm {
		R	= DataSpaceDesc::R,
		W	= DataSpaceDesc::W,
		X	= DataSpaceDesc::X,
		M	= 1 << 3,	// mapped
		RW	= R | W,
		RX	= R | X,
		RWX	= R | W | X,
	};

	explicit ChildMemory() : _regs(), _ds() {
	}

	ds_iterator ds_begin() const {
		return _ds;
	}
	ds_iterator ds_end() const {
		return _ds + MAX_DS;
	}

	size_t regcount() const {
		size_t count = 0;
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size > 0)
				count++;
		}
		return count;
	}

	bool find(capsel_t sel,DataSpaceDesc &ds) {
		size_t i = get(sel);
		if(i == MAX_DS)
			return false;
		ds = _ds[i].desc;
		return true;
	}

	uint find(uintptr_t addr,uintptr_t &src,size_t &size) const {
		addr &= ~(ExecEnv::PAGE_SIZE - 1);
		const Region *r = get(addr,1);
		if(r) {
			src = r->src + (addr - r->begin);
			size = r->size - (addr - r->begin);
			return r->flags;
		}
		return 0;
	}

	uintptr_t find_free(size_t size) const {
		// find the end of the "highest" region
		uintptr_t end = 0;
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size > 0 && _regs[i].begin + _regs[i].size > end)
				end = _regs[i].begin + _regs[i].size;
		}
		// round up to next page
		end = (end + ExecEnv::PAGE_SIZE - 1) & ~(ExecEnv::PAGE_SIZE - 1);
		// check if the size fits below the kernel
		if(end + size < end || end + size > ExecEnv::KERNEL_START)
			throw ChildMemoryException(E_CAPACITY);
		return end;
	}

	void map(uintptr_t addr,size_t size = ExecEnv::PAGE_SIZE) {
		const Region *r = get(addr,1);
		assert(r);
		add(addr,size,r->src + addr - r->begin,r->flags | M);
	}

	void unmap(uintptr_t addr,size_t size = ExecEnv::PAGE_SIZE) {
		const Region *r = get(addr,1);
		assert(r);
		add(addr,size,r->src + addr - r->begin,r->flags & ~M);
	}

	void add(const DataSpaceDesc& desc,uintptr_t addr,uint perm,capsel_t ds) {
		for(size_t i = 0; i < MAX_DS; ++i) {
			if(_ds[i].unmapsel == 0) {
				_ds[i].unmapsel = ds;
				_ds[i].desc = DataSpaceDesc(desc.size(),desc.type(),desc.perm(),desc.phys(),addr,desc.virt());
				add(addr,desc.size(),desc.virt(),perm);
				return;
			}
		}
		throw ChildMemoryException(E_CAPACITY);
	}

	DataSpaceDesc remove(capsel_t ds) {
		size_t i = get(ds);
		if(i == MAX_DS)
			throw ChildMemoryException(E_NOT_FOUND);
		_ds[i].unmapsel = 0;
		remove(_ds[i].desc.virt(),_ds[i].desc.size());
		return _ds[i].desc;
	}

	void add(uintptr_t addr,size_t size,uintptr_t src,uint flags) {
		Region *r = get(addr,size);
		if(r) {
			// if its the simple case that it matches the complete region, just exchange the attributes
			if(addr == r->begin && size == r->size) {
				r->src = src;
				r->flags = flags;
				return;
			}
			// otherwise remove this range and add it again with updated flags
			remove(addr,size);
		}

		r = get_free();
		r->src = src;
		r->begin = addr;
		r->size = size;
		r->flags = flags;
	}

	void remove(uintptr_t addr,size_t size) {
		Region *r;
		while((r = get(addr,size))) {
			// at the beginning?
			if(addr <= r->begin) {
				// complete region
				if(addr + size >= r->begin + r->size)
					r->size = 0;
				// beginning of region
				else {
					r->size = r->begin + r->size - (addr + size);
					r->src += (addr + size) - r->begin;
					r->begin = addr + size;
				}
			}
			else {
				// complete end of region
				if(addr + size >= r->begin + r->size)
					r->size = addr - r->begin;
				// somewhere in the middle
				else {
					Region *nr = get_free();
					nr->begin = addr + size;
					nr->size = r->begin + r->size - nr->begin;
					nr->flags = r->flags;
					nr->src = r->src + (nr->begin - r->begin);
					r->size = addr - r->begin;
				}
			}
		}
	}

private:
	size_t get(capsel_t ds) {
		for(size_t i = 0; i < MAX_DS; ++i) {
			if(_ds[i].unmapsel == ds)
				return i;
		}
		return MAX_DS;
	}
	Region *get(uintptr_t addr,size_t size) {
		return const_cast<Region*>(const_cast<const ChildMemory*>(this)->get(addr,size));
	}
	const Region *get(uintptr_t addr,size_t size) const {
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			// TODO overlapped is wrong here?
			if(_regs[i].size > 0 && Math::overlapped(addr,size,_regs[i].begin,_regs[i].size))
				return _regs + i;
		}
		return 0;
	}
	Region *get_free() {
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size == 0)
				return _regs + i;
		}
		throw ChildMemoryException(E_CAPACITY);
	}

	Region _regs[MAX_REGIONS];
	DS _ds[MAX_DS];
};

}
