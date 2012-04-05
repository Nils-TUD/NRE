/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <kobj/LocalEc.h>

extern "C" void idc_reply_and_wait_fast(void*);

void *LocalEc::create_stack() {
	unsigned stack_top = sizeof(ec_stacks[ec_stack]) / sizeof(void*);
	ec_stacks[ec_stack][--stack_top] = this; // put Ec instance on stack-top
	ec_stacks[ec_stack][--stack_top] = reinterpret_cast<void*>(0xDEADBEEF);
	ec_stacks[ec_stack][--stack_top] = reinterpret_cast<void*>(idc_reply_and_wait_fast);
	return ec_stacks[ec_stack++] + stack_top;
}
