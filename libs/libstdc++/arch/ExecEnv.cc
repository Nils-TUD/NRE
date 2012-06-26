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

#include <arch/ExecEnv.h>
#include <kobj/UserSm.h>
#include <util/ScopedLock.h>
#include <Compiler.h>
#include <util/Math.h>

namespace nul {

void *ExecEnv::setup_stack(Pd *pd,Ec *ec,startup_func start,uintptr_t ret,uintptr_t stack) {
	void **sp = reinterpret_cast<void**>(stack);
	unsigned stack_top = STACK_SIZE / sizeof(void*);
	sp[--stack_top] = ec;
	sp[--stack_top] = pd;
	sp[--stack_top] = reinterpret_cast<void*>(start);
	sp[--stack_top] = reinterpret_cast<void*>(ret);
	return sp + stack_top;
}

size_t ExecEnv::collect_backtrace(uintptr_t *frames,size_t max) {
	uintptr_t bp;
	asm volatile ("mov %%" EXPAND(REG(bp)) ",%0" : "=a" (bp));
	return collect_backtrace(Math::round_dn<uintptr_t>(bp,ExecEnv::STACK_SIZE),bp,frames,max);
}

size_t ExecEnv::collect_backtrace(uintptr_t stack,uintptr_t bp,uintptr_t *frames,size_t max) {
	uintptr_t end,start;
	uintptr_t *frame = frames;
	size_t count = 0;
	end = Math::round_up<uintptr_t>(bp,ExecEnv::STACK_SIZE);
	start = end - ExecEnv::STACK_SIZE;
	for(size_t i = 0; i < max - 1; i++) {
		// prevent page-fault
		if(bp < start || bp >= end)
			break;
		bp = stack + (bp & (ExecEnv::STACK_SIZE - 1));
		*frame = *(reinterpret_cast<uintptr_t*>(bp) + 1) - CALL_INSTR_SIZE;
		frame++;
		count++;
		bp = *reinterpret_cast<uintptr_t*>(bp);
	}
	// terminate
	*frame = 0;
	return count;
}

}
