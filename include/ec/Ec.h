/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <Types.h>
#include <Utcb.h>

class Pd;

class Ec {
public:
	enum {
		// TODO grab pagesize from HIP
		STACK_SIZE = 4096
	};

	static Ec *current() {
		uint32_t esp;
	    asm volatile ("mov %%esp, %0" : "=g"(esp));
	    return *reinterpret_cast<Ec**>(((esp & ~(STACK_SIZE - 1)) + STACK_SIZE - 1 * sizeof(void*)));
	}

	Ec(cpu_t cpu) : _utcb(0), _cpu(cpu) {
	}
	virtual ~Ec() {
	}

	Pd *pd() {
		return _pd;
	}
	Utcb *utcb() {
		return _utcb;
	}
	cpu_t cpu() const {
		return _cpu;
	}

private:
	Ec(const Ec&);
	Ec& operator=(const Ec&);

private:
	Utcb *_utcb;
	Pd *_pd;
	cpu_t _cpu;
};
