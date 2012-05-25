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
#include <ex/Exception.h>
#include <stream/OStream.h>
#include <mem/DataSpace.h>
#include <Util.h>
#include <assert.h>

extern void test_reglist();

namespace nul {

class RegionList {
	friend void ::test_reglist();

	enum {
		MAX_REGIONS		= 64,
		MAX_DS			= 32
	};

	struct DSInst {
		uintptr_t addr;
		capsel_t unmapsel;
	};
	struct Region {
		uintptr_t src;
		uintptr_t begin;
		size_t size;
		uint flags;
	};

public:
	enum Perm {
		R	= DataSpace::R,
		W	= DataSpace::W,
		X	= DataSpace::X,
		M	= 1 << 3,	// mapped
		RW	= R | W,
		RX	= R | X,
		RWX	= R | W | X,
	};

	RegionList() : _regs(), _ds() {
	}

	size_t regcount() const {
		size_t count = 0;
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size > 0)
				count++;
		}
		return count;
	}

	uint find(uintptr_t addr,uintptr_t &src) const {
		addr &= ~(ExecEnv::PAGE_SIZE - 1);
		const Region *r = get(addr,1);
		if(r) {
			src = r->src + (addr - r->begin);
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
			throw Exception("Not enough space");
		return end;
	}

	void map(uintptr_t addr) {
		const Region *r = get(addr,1);
		assert(r);
		add(addr,ExecEnv::PAGE_SIZE,r->src + addr - r->begin,r->flags | M);
	}

	void unmap(uintptr_t addr) {
		const Region *r = get(addr,1);
		assert(r);
		add(addr,ExecEnv::PAGE_SIZE,r->src + addr - r->begin,r->flags & ~M);
	}

	void add(const DataSpace& ds,uintptr_t addr) {
		for(size_t i = 0; i < MAX_DS; ++i) {
			if(_ds[i].unmapsel == 0) {
				_ds[i].unmapsel = ds.unmapsel();
				_ds[i].addr = addr;
				add(addr,ds.size(),ds.virt(),ds.perm());
				return;
			}
		}
		throw Exception("No free dataspace slot");
	}

	void remove(const DataSpace& ds) {
		for(size_t i = 0; i < MAX_DS; ++i) {
			if(_ds[i].unmapsel == ds.unmapsel()) {
				_ds[i].unmapsel = 0;
				remove(_ds[i].addr,ds.size());
				return;
			}
		}
		throw Exception("Dataspace not found");
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

	void write(OStream &os) const {
		os.writef("Regions:\n");
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size > 0) {
				os.writef(
					"\t%zu: %p .. %p (%zu bytes) : %c%c%c%c, src=%p\n",i,_regs[i].begin,
						_regs[i].begin + _regs[i].size,_regs[i].size,
						(_regs[i].flags & R) ? 'r' : '-',
						(_regs[i].flags & W) ? 'w' : '-',
						(_regs[i].flags & X) ? 'x' : '-',
						(_regs[i].flags & M) ? 'm' : '-',
						_regs[i].src
				);
			}
		}
	}

private:
	Region *get(uintptr_t addr,size_t size) {
		return const_cast<Region*>(const_cast<const RegionList*>(this)->get(addr,size));
	}
	const Region *get(uintptr_t addr,size_t size) const {
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size > 0 && Util::overlapped(addr,size,_regs[i].begin,_regs[i].size))
				return _regs + i;
		}
		return 0;
	}
	Region *get_free() {
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size == 0)
				return _regs + i;
		}
		throw Exception("No free regions");
	}

	Region _regs[MAX_REGIONS];
	DSInst _ds[MAX_DS];
};

}
