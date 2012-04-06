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

#include <kobj/LocalEc.h>

extern "C" void idc_reply_and_wait_fast(void*);

void *LocalEc::create_stack() {
	unsigned stack_top = sizeof(ec_stacks[ec_stack]) / sizeof(void*);
	ec_stacks[ec_stack][--stack_top] = this; // put Ec instance on stack-top
	ec_stacks[ec_stack][--stack_top] = reinterpret_cast<void*>(0xDEADBEEF);
	ec_stacks[ec_stack][--stack_top] = reinterpret_cast<void*>(idc_reply_and_wait_fast);
	return ec_stacks[ec_stack++] + stack_top;
}
