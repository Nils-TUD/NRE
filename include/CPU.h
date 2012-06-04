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
#include <Assert.h>

namespace nul {

class Pt;
class Sm;

class CPU {
public:
	static CPU &current() {
		return get(Ec::current()->cpu());
	}
	static CPU &get(cpu_t id) {
		assert(id < Hip::MAX_CPUS);
		return cpus[id];
	}

	cpu_t id;
	Pt *map_pt;
	Pt *unmap_pt;
	Pt *allocio_pt;
	Pt *reg_pt;
	Pt *get_pt;
	Sm *srv_sm;

private:
	CPU() : id(), map_pt(), unmap_pt(), allocio_pt(), reg_pt(), get_pt(), srv_sm() {
	}
	CPU(const CPU&);
	CPU& operator=(const CPU&);

	static CPU cpus[Hip::MAX_CPUS];
};

}
