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
#include <arch/Startup.h>
#include <kobj/Ec.h>
#include <kobj/LocalEc.h>

namespace nul {

class Utcb;

class GlobalEc : public Ec {
	friend void ::_presetup();

	explicit GlobalEc(Utcb *utcb,capsel_t cap,cpu_t cpu,Pd *pd) : Ec(cpu,0,cap,utcb) {
		ExecEnv::set_current_ec(this);
		ExecEnv::set_current_pd(pd);
	}

public:
	typedef ExecEnv::startup_func startup_func;

	explicit GlobalEc(startup_func start,cpu_t cpu,Pd *pd = Pd::current(),uintptr_t utcb = 0)
			: Ec(cpu,Hip::get().service_caps() * cpu,INVALID,reinterpret_cast<Utcb*>(utcb)) {
		create(pd,Syscalls::EC_GLOBAL,ExecEnv::setup_stack(pd,this,start));
	}
	explicit GlobalEc(startup_func start,cpu_t cpu,capsel_t event_base,Pd *pd = Pd::current(),
			uintptr_t utcb = 0) : Ec(cpu,event_base,INVALID,reinterpret_cast<Utcb*>(utcb)) {
		create(pd,Syscalls::EC_GLOBAL,ExecEnv::setup_stack(pd,this,start));
	}
};

}
