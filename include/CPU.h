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
#include <Assert.h>
#include <SList.h>
#include <Hip.h>

namespace nul {

class Pt;
class CPUInit;

class CPU {
	friend class CPUInit;

public:
	typedef SListIterator<CPU> iterator;

	static CPU &current() {
		return get(Ec::current()->cpu());
	}
	static CPU &get(cpu_t id) {
		assert(id < Hip::MAX_CPUS);
		return _cpus[id];
	}

	static iterator begin() {
		return SListIterator<CPU>(_online);
	}
	static iterator end() {
		return SListIterator<CPU>();
	}

	cpu_t id;
	uint8_t flags;
	uint8_t thread;
	uint8_t core;
	uint8_t package;

	Pt *map_pt;
	Pt *unmap_pt;
	Pt *io_pt;
	Pt *gsi_pt;
	Pt *reg_pt;
	Pt *unreg_pt;
	Pt *get_pt;

	CPU *next() {
		return _next;
	}

private:
	CPU() : id(), map_pt(), unmap_pt(), io_pt(), gsi_pt(), reg_pt(), unreg_pt(), get_pt(), _next() {
	}
	CPU(const CPU&);
	CPU& operator=(const CPU&);

	CPU *_next;
	static CPU *_online;
	static CPU _cpus[Hip::MAX_CPUS];
};

}
