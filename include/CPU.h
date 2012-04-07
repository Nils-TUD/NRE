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

#include <Hip.h>
#include <assert.h>

class LocalEc;
class Pt;

class CPU {
public:
	static CPU &get(cpu_t id) {
		assert(id < Hip::MAX_CPUS);
		return cpus[id];
	}

	CPU() : id(), map_pt(), ec() {
	}

	cpu_t id;
	Pt *map_pt;
	LocalEc * ec;

private:
	static CPU cpus[Hip::MAX_CPUS];
};
