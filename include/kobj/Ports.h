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

#include <utcb/UtcbFrame.h>
#include <cap/CapHolder.h>
#include <kobj/Pt.h>
#include <CPU.h>
#include <Util.h>

namespace nul {

class Ports {
public:
	typedef uint port_t;

	enum Op {
		ALLOC,
		RELEASE
	};

	explicit Ports(port_t base,uint count) : _base(base), _count(count) {
		UtcbFrame uf;
		CapHolder cap;
		uf.set_receive_crd(Crd(_base,Math::next_pow2_shift(_count),Crd::IO_ALL));
		uf << ALLOC << _base << _count;
		CPU::current().io_pt->call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
	}
	~Ports() {
		UtcbFrame uf;
		uf << RELEASE << _base << _count;
		CPU::current().io_pt->call(uf);
	}

	port_t base() const {
		return _base;
	}
	uint count() const {
		return _count;
	}

	template<typename T>
	inline T in(port_t offset = 0) {
		assert(offset < _count);
		T val;
		asm volatile ("in %w1, %0" : "=a" (val) : "Nd" (_base + offset));
		return val;
	}

    template<typename T>
	inline void out(T val,port_t offset = 0) {
		assert(offset < _count);
		asm volatile ("out %0, %w1" : : "a" (val), "Nd" (_base + offset));
	}

private:
    Ports(const Ports&);
    Ports& operator=(const Ports&);

    port_t _base;
    uint _count;
};

}
