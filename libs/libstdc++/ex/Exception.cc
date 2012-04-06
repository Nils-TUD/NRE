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

#include <ex/Exception.h>
#include <kobj/Ec.h>

// the x86-call instruction is 5 bytes long
#define CALL_INSTR_SIZE		5

Exception::Exception(const char *msg) throw() : _msg(msg), _backtrace(), _count(0) {
	// TODO this is architecture dependent
	uintptr_t end,start;
	uint32_t *ebp;
	uintptr_t *frame = &_backtrace[0];
	asm volatile ("mov %%ebp,%0" : "=a" (ebp));
	end = ((uintptr_t)ebp + (Ec::STACK_SIZE - 1)) & ~(Ec::STACK_SIZE - 1);
	start = end - Ec::STACK_SIZE;
	for(size_t i = 0; i < MAX_TRACE_DEPTH - 1; i++) {
		// prevent page-fault
		if((uintptr_t)ebp < start || (uintptr_t)ebp >= end)
			break;
		*frame = *(ebp + 1) - CALL_INSTR_SIZE;
		frame++;
		_count++;
		ebp = (uint32_t*)*ebp;
	}
	// terminate
	*frame = 0;
}
