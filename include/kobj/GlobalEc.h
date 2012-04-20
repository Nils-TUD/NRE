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

#include <arch/ExecutionEnv.h>
#include <arch/Startup.h>
#include <kobj/Ec.h>
#include <kobj/LocalEc.h>
#include <CPU.h>

namespace nul {

class Utcb;

class GlobalEc : public Ec {
	friend void ::_setup();

	explicit GlobalEc(Utcb *utcb,cap_t cap,cpu_t cpu,Pd *pd) : Ec(cpu,0,cap,utcb) {
		ExecutionEnv::set_current_ec(this);
		ExecutionEnv::set_current_pd(pd);
	}

public:
	typedef ExecutionEnv::startup_func startup_func;

	explicit GlobalEc(startup_func start,cpu_t cpu,Pd *pd = Pd::current())
			: Ec(cpu,CPU::get(cpu).ec->event_base()) {
		create(pd,Syscalls::EC_GLOBAL,ExecutionEnv::setup_stack(pd,this,start));
	}
};

}
