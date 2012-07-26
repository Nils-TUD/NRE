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

#include <utcb/UtcbFrame.h>
#include <util/ScopedCapSels.h>
#include <kobj/Pt.h>
#include <CPU.h>
#include <util/Util.h>

namespace nre {

/**
 * Represents I/O ports that are allocated from the parent and released again on destruction.
 */
class Ports {
public:
	typedef uint port_t;

	enum Op {
		ALLOC,
		RELEASE
	};

	/**
	 * Allocates the given port range from the parent.
	 *
	 * @param base the beginning of the port range
	 * @param count the number of ports
	 * @throws Exception if failed (e.g. ports already allocated)
	 */
	explicit Ports(port_t base,uint count) : _base(base), _count(count) {
		alloc();
	}
	/**
	 * Releases the ports again
	 */
	~Ports() {
		release();
	}

	/**
	 * @return the base of the port-range
	 */
	port_t base() const {
		return _base;
	}
	/**
	 * @return the number of ports
	 */
	uint count() const {
		return _count;
	}

	/**
	 * Reads a value from port base()+<offset>.
	 *
	 * @param offset the offset within the port-range
	 * @return the value
	 */
	template<typename T>
	inline T in(port_t offset = 0) {
		assert(offset < _count);
		T val;
		asm volatile ("in %w1, %0" : "=a" (val) : "Nd" (_base + offset));
		return val;
	}

	/**
	 * Writes <val> to port base()+<offset>
	 *
	 * @param val the value to write
	 * @param offset the offset within the port-range
	 */
	template<typename T>
	inline void out(T val,port_t offset = 0) {
		assert(offset < _count);
		asm volatile ("out %0, %w1" : : "a" (val), "Nd" (_base + offset));
	}

private:
	void alloc() {
    	UtcbFrame uf;
		ScopedCapSels cap;
		uf.delegation_window(Crd(_base,Math::next_pow2_shift(_count),Crd::IO_ALL));
		uf << ALLOC << _base << _count;
		CPU::current().io_pt().call(uf);
		uf.check_reply();
	}
	void release() {
		try {
			UtcbFrame uf;
			uf << RELEASE << _base << _count;
			CPU::current().io_pt().call(uf);
		}
		catch(...) {
			// ignore
		}
	}

	Ports(const Ports&);
	Ports& operator=(const Ports&);

	port_t _base;
	uint _count;
};

}
