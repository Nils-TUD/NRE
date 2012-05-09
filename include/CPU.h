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
#include <cap/CapSpace.h>
#include <Hip.h>
#include <assert.h>

namespace nul {

class Pt;

class CPU {
public:
	static CPU &current() {
		return get(Ec::current()->cpu());
	}
	static CPU &get(cpu_t id) {
		assert(id < Hip::MAX_CPUS);
		return cpus[id];
	}

	CPU() : id(), map_pt(), reg_pt(), get_pt() {
	}

	cpu_t id;
	Pt *map_pt;
	Pt *reg_pt;
	Pt *get_pt;

private:
	static CPU cpus[Hip::MAX_CPUS];
};

}
