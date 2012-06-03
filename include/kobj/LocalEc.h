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

EXTERN_C void portal_reply_landing_spot(void*);

namespace nul {

class LocalEc : public Ec {
public:
	explicit LocalEc(cpu_t cpu,capsel_t event_base = INVALID,uintptr_t stackaddr = 0)
			: Ec(cpu,event_base == INVALID ? Hip::get().service_caps() * cpu : event_base,INVALID,0,stackaddr) {
		Pd *pd = Pd::current();
		create(pd,Syscalls::EC_LOCAL,ExecEnv::setup_stack(pd,this,0,
				reinterpret_cast<uintptr_t>(portal_reply_landing_spot),stack().virt()));
	}
	virtual ~LocalEc() {
	}
};

}
