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
#include <CPU.h>

extern "C" void portal_reply_landing_spot(void);

namespace nul {

class LocalEc : public Ec {
public:
	explicit LocalEc(cpu_t cpu) : Ec(cpu,CPU::get(cpu).ec->event_base()) {
		Pd *pd = Pd::current();
		create(pd,Syscalls::EC_LOCAL,ExecEnv::setup_stack(pd,this,portal_reply_landing_spot));
	}
	explicit LocalEc(cpu_t cpu,cap_t event_base) : Ec(cpu,event_base) {
		Pd *pd = Pd::current();
		create(pd,Syscalls::EC_LOCAL,ExecEnv::setup_stack(pd,this,portal_reply_landing_spot));
	}
	virtual ~LocalEc() {
	}
};

}
