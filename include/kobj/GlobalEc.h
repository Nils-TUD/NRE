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

#include <kobj/Ec.h>
#include <kobj/LocalEc.h>
#include <CPU.h>

class Utcb;
class Hip;

extern "C" int start(cpu_t cpu,Utcb *utcb);

class GlobalEc : public Ec {
	friend int start(cpu_t cpu,Utcb *utcb);

public:
	typedef void (*func_t)();

private:
	static void set_current(Ec *ec) {
		uint32_t esp;
	    asm volatile ("mov %%esp, %0" : "=g"(esp));
	    *reinterpret_cast<Ec**>(((esp & ~(STACK_SIZE - 1)) + STACK_SIZE - 1 * sizeof(void*))) = ec;
	}

	explicit GlobalEc(Utcb *utcb,cap_t cap,cpu_t cpu,Pd *pd) : Ec(cpu,pd,0,cap,utcb) {
	}

public:
	explicit GlobalEc(func_t start,cpu_t cpu,Pd *pd = Pd::current())
			: Ec(cpu,pd,CPU::get(cpu).ec->event_base()) {
		create(Syscalls::EC_GENERAL,create_stack(start));
	}

private:
	void *create_stack(func_t start) {
		unsigned stack_top = sizeof(ec_stacks[ec_stack]) / sizeof(void*);
		ec_stacks[ec_stack][--stack_top] = this; // put Ec instance on stack-top
		ec_stacks[ec_stack][--stack_top] = reinterpret_cast<void*>(0xDEADBEEF);
		ec_stacks[ec_stack][--stack_top] = reinterpret_cast<void*>(start);
		return ec_stacks[ec_stack++] + stack_top;
	}
};
