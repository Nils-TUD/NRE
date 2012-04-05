/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <ec/LocalEc.h>

extern "C" void idc_reply_and_wait_fast(void*);

void *LocalEc::create_stack() {
	unsigned stack_top = sizeof(_stacks[_stack]) / sizeof(void*);
	_stacks[_stack][--stack_top] = this; // put Ec instance on stack-top
	_stacks[_stack][--stack_top] = reinterpret_cast<void*>(0xDEADBEEF);
	_stacks[_stack][--stack_top] = reinterpret_cast<void*>(idc_reply_and_wait_fast);
	return _stacks[_stack++] + stack_top;
}
