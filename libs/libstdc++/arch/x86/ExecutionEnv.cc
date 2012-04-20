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

#include <arch/x86/ExecutionEnv.h>
#include <Compiler.h>

namespace nul {

void *ExecutionEnv::_stacks[MAX_STACKS][STACK_SIZE / sizeof(void*)] ALIGNED(ExecutionEnv::PAGE_SIZE);
size_t ExecutionEnv::_stack;

void *ExecutionEnv::setup_stack(Pd *pd,Ec *ec,startup_func start) {
	unsigned stack_top = sizeof(_stacks[_stack]) / sizeof(void*);
	_stacks[_stack][--stack_top] = ec;
	_stacks[_stack][--stack_top] = pd;
	_stacks[_stack][--stack_top] = reinterpret_cast<void*>(0xDEADBEEF);
	_stacks[_stack][--stack_top] = reinterpret_cast<void*>(start);
	return _stacks[_stack++] + stack_top;
}

size_t ExecutionEnv::collect_backtrace(uintptr_t *frames,size_t max) {
	uintptr_t end,start;
	uint32_t *ebp;
	uintptr_t *frame = frames;
	size_t count = 0;
	asm volatile ("mov %%ebp,%0" : "=a" (ebp));
	end = ((uintptr_t)ebp + (ExecutionEnv::STACK_SIZE - 1)) & ~(ExecutionEnv::STACK_SIZE - 1);
	start = end - ExecutionEnv::STACK_SIZE;
	for(size_t i = 0; i < max - 1; i++) {
		// prevent page-fault
		if((uintptr_t)ebp < start || (uintptr_t)ebp >= end)
			break;
		*frame = *(ebp + 1) - CALL_INSTR_SIZE;
		frame++;
		count++;
		ebp = (uint32_t*)*ebp;
	}
	// terminate
	*frame = 0;
	return count;
}

}
