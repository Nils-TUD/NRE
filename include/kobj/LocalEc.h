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

#include <kobj/Ec.h>
#include <CPU.h>

class LocalEc : public Ec {
public:
	typedef void (*portal_func)(unsigned) __attribute__((regparm(1)));

	explicit LocalEc(cpu_t cpu,cap_t event_base) : Ec(cpu,Pd::current(),event_base) {
		create(Syscalls::EC_WORKER,create_stack());
	}
	explicit LocalEc(cpu_t cpu) : Ec(cpu,Pd::current(),CPU::get(cpu).ec->event_base()) {
		create(Syscalls::EC_WORKER,create_stack());
	}
	virtual ~LocalEc() {
	}

	cap_t create_portal(portal_func func,unsigned mtd) {
		CapHolder ptcap(pd()->obj());
		Syscalls::create_pt(ptcap.get(),cap(),reinterpret_cast<uintptr_t>(func),mtd,pd()->cap());
		return ptcap.release();
	}
	void create_portal_for(cap_t pt,portal_func func,unsigned mtd) {
		Syscalls::create_pt(event_base() + pt,cap(),reinterpret_cast<uintptr_t>(func),mtd,pd()->cap());
	}

private:
	void *create_stack();
};
