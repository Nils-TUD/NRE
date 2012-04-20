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

#include <Compiler.h>
#include <Types.h>

#define PORTAL	REGPARMS(1)

namespace nul {

class Ec;
class Pd;

class ExecutionEnv {
	enum {
		// the x86-call instruction is 5 bytes long
		CALL_INSTR_SIZE	= 5
	};

public:
	typedef PORTAL void (*portal_func)(unsigned);
	typedef void (*startup_func)();

	enum {
		PAGE_SIZE = 4096,
		STACK_SIZE = PAGE_SIZE,
		MAX_STACKS = 8	// TODO remove
	};

	static inline Pd *get_current_pd() {
		return static_cast<Pd*>(get_current(2));
	}

	static inline void set_current_pd(Pd *pd) {
		set_current(2,pd);
	}

	static inline Ec *get_current_ec() {
		return static_cast<Ec*>(get_current(1));
	}

	static inline void set_current_ec(Ec *ec) {
		set_current(1,ec);
	}

	static void *setup_stack(Pd *pd,Ec *ec,startup_func start);
	static size_t collect_backtrace(uintptr_t *frames,size_t max);

private:
	ExecutionEnv();
	~ExecutionEnv();
	ExecutionEnv(const ExecutionEnv&);
	ExecutionEnv& operator=(const ExecutionEnv&);

	static inline void *get_current(size_t no) {
		uint32_t esp;
	    asm volatile ("mov %%esp, %0" : "=g"(esp));
	    return *reinterpret_cast<void**>(((esp & ~(STACK_SIZE - 1)) + STACK_SIZE - no * sizeof(void*)));
	}

	static inline void set_current(size_t  no,void *obj) {
		uint32_t esp;
	    asm volatile ("mov %%esp, %0" : "=g"(esp));
	    *reinterpret_cast<void**>(((esp & ~(STACK_SIZE - 1)) + STACK_SIZE - no * sizeof(void*))) = obj;
	}

	static void *_stacks[MAX_STACKS][STACK_SIZE / sizeof(void*)];
	static size_t _stack;
};

}
