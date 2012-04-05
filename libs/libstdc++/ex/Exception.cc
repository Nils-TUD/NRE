/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <ex/Exception.h>
#include <kobj/Ec.h>

// the x86-call instruction is 5 bytes long
#define CALL_INSTR_SIZE		5

Exception::Exception() throw() : _backtrace(), _count(0) {
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
