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
#include <kobj/Ec.h>
#include <CPU.h>

extern "C" void idc_reply_and_wait_fast(void);

namespace nul {

class LocalEc : public Ec {
public:
	explicit LocalEc(cpu_t cpu) : Ec(cpu,Pd::current(),CPU::get(cpu).ec->event_base()) {
		create(Syscalls::EC_LOCAL,ExecutionEnv::setup_stack(this,idc_reply_and_wait_fast));
	}
	explicit LocalEc(cpu_t cpu,cap_t event_base) : Ec(cpu,Pd::current(),event_base) {
		create(Syscalls::EC_LOCAL,ExecutionEnv::setup_stack(this,idc_reply_and_wait_fast));
	}
	virtual ~LocalEc() {
	}
};

}
