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

#include <arch/ExecEnv.h>
#include <kobj/Ec.h>
#include <Compiler.h>

/**
 * The return address for portals, which will then do the reply syscall.
 */
EXTERN_C void portal_reply_landing_spot(void*);

namespace nul {

/**
 * A LocalEc is an execution-context that is only used to serve portal-calls. Therefore, it doesn't
 * have an entry-point, but you can bind portals to it.
 */
class LocalEc : public Ec {
public:
	/**
	 * Creates a new LocalEc on CPU <cpu>.
	 *
	 * @param cpu the logical CPU id to bind the Ec to
	 * @param event_base the event-base to use for this Ec (default: select it automatically)
	 * @param stackaddr the stack-address (0 = create a stack)
	 * @param utcb the utcb-address (0 = select it automatically)
	 */
	explicit LocalEc(cpu_t cpu,capsel_t event_base = INVALID,uintptr_t stackaddr = 0,uintptr_t utcb = 0)
			: Ec(cpu,event_base == INVALID ? Hip::get().service_caps() * cpu : event_base,
					INVALID,stackaddr,utcb) {
		Pd *pd = Pd::current();
		create(pd,Syscalls::EC_LOCAL,ExecEnv::setup_stack(pd,this,0,
				reinterpret_cast<uintptr_t>(portal_reply_landing_spot),stack()));
	}
};

}
