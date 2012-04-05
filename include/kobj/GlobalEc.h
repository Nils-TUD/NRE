/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <kobj/Ec.h>

class Utcb;
class Hip;

extern "C" int start(cpu_t cpu,Utcb *utcb,Hip *hip);

class GlobalEc : public Ec {
	friend int start(cpu_t cpu,Utcb *utcb,Hip *hip);

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
	explicit GlobalEc(func_t start,cpu_t cpu,Pd *pd = Pd::current()) : Ec(cpu,pd) {
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
