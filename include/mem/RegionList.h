/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <ex/Exception.h>
#include <format/Format.h>
#include <Types.h>

namespace nul {

class RegionList {
	enum {
		MAX_REGIONS = 64
	};

	struct Region {
		uintptr_t src;
		uintptr_t begin;
		size_t size;
		uint flags;
	};

public:
	enum Perm {
		// note that this equals the values in a Crd
		R	= 1 << 0,
		W	= 1 << 1,
		X	= 1 << 2,
		M	= 1 << 3,	// mapped
		RW	= R | W,
		RX	= R | X,
		RWX	= R | W | X,
	};

	uint find(uintptr_t addr,uintptr_t *src) const {
		const Region *r = get(addr,1);
		if(r) {
			*src = r->src + (addr - r->begin);
			return r->flags;
		}
		return 0;
	}

	uintptr_t find_free(size_t size) const {
		uintptr_t end = 0;
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size > 0 && _regs[i].begin + _regs[i].size > end)
				end = _regs[i].begin + _regs[i].size;
		}
		return (end + ExecEnv::PAGE_SIZE - 1) & ~(ExecEnv::PAGE_SIZE - 1);
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

	void add(uintptr_t addr,size_t size,uintptr_t src,uint flags) {
		Region *r = get(addr,size);
		if(r) {
			if(addr == r->begin && size == r->size) {
				r->src = src;
				r->flags = flags;
				return;
			}
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

	void print(Format &fmt) const {
		fmt.print("Regions:\n");
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size > 0) {
				fmt.print(
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
};

}
