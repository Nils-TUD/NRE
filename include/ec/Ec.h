/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <pd/CapHolder.h>
#include <pd/Pd.h>
#include <Syscalls.h>
#include <Utcb.h>

class Ec {
public:
	enum {
		// TODO grab pagesize from HIP
		STACK_SIZE = 4096,
		MAX_STACKS = 8
	};

	static Ec *current() {
		uint32_t esp;
	    asm volatile ("mov %%esp, %0" : "=g"(esp));
	    return *reinterpret_cast<Ec**>(((esp & ~(STACK_SIZE - 1)) + STACK_SIZE - 1 * sizeof(void*)));
	}

protected:
	Ec(cpu_t cpu,Pd *pd,cap_t event_base = 0,cap_t cap = 0,Utcb *utcb = 0)
		: _utcb(utcb), _pd(pd), _event_base(event_base), _cap(cap), _cpu(cpu) {
		if(!_utcb) {
			// TODO
			_utcb = reinterpret_cast<Utcb*>(_utcb_addr);
			_utcb_addr -= 0x1000;
		}
	}
	void create(Syscalls::ECType type,void *sp) {
		CapHolder cap(_pd->obj());
		Syscalls::create_ec(cap.get(),_utcb,sp,_cpu,_event_base,type,_pd->cap());
		_cap = cap.release();
	}

public:
	virtual ~Ec() {
	}

	cap_t cap() const {
		return _cap;
	}
	cap_t event_base() const {
		return _event_base;
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
	cap_t _event_base;
	cap_t _cap;
	cpu_t _cpu;

	// TODO
protected:
	static void *_stacks[MAX_STACKS][STACK_SIZE / sizeof(void*)] __attribute__((aligned(STACK_SIZE)));
	static size_t _stack;
	static uintptr_t _utcb_addr;
};
