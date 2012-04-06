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
	explicit Ec(cpu_t cpu,Pd *pd,cap_t event_base,cap_t cap = INVALID,Utcb *utcb = 0)
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
		// TODO stack, utcb
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
