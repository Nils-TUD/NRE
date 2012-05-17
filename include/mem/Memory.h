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
#include <ex/Exception.h>
#include <stream/OStream.h>

namespace nul {

// TODO thread-safety?
class Memory {
	enum {
		MAX_REGIONS		= 64
	};

	struct Region {
		uintptr_t addr;
		size_t size;
	};

	Memory() : _regs() {
	}

public:
	static Memory &get() {
		return _inst;
	}

	uintptr_t alloc(size_t size) {
		Region *r = get(size);
		if(!r)
			throw Exception("Not enough space");
		r->addr += size;
		r->size -= size;
		return r->addr - size;
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
					"\t%zu: %p .. %p (%zu bytes)\n",i,_regs[i].addr,
						_regs[i].addr + _regs[i].size,_regs[i].size
				);
			}
		}
	}

private:
	Region *get(size_t size) {
		for(size_t i = 0; i < MAX_REGIONS; ++i) {
			if(_regs[i].size >= size)
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
	static Memory _inst;
};

}
