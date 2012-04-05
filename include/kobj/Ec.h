/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <cap/CapHolder.h>
#include <kobj/KObject.h>
#include <kobj/Pd.h>
#include <Syscalls.h>
#include <Utcb.h>

class Ec : public KObject {
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
	explicit Ec(cpu_t cpu,Pd *pd,cap_t event_base = 0,cap_t cap = INVALID,Utcb *utcb = 0)
			: KObject(pd,cap), _utcb(utcb), _event_base(event_base), _cpu(cpu) {
		if(!_utcb) {
			// TODO
			_utcb = reinterpret_cast<Utcb*>(_utcb_addr);
			_utcb_addr -= 0x1000;
		}
	}
	void create(Syscalls::ECType type,void *sp) {
		CapHolder ch(pd()->obj());
		Syscalls::create_ec(ch.get(),_utcb,sp,_cpu,_event_base,type,pd()->cap());
		cap(ch.release());
	}

public:
	virtual ~Ec() {
	}

	cap_t event_base() const {
		return _event_base;
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
	cap_t _event_base;
	cpu_t _cpu;

	// TODO
protected:
	static uintptr_t _utcb_addr;
};

extern void *ec_stacks[Ec::MAX_STACKS][Ec::STACK_SIZE / sizeof(void*)];
extern size_t ec_stack;
