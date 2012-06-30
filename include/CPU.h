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
#include <util/SList.h>
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
	static CPU &get(cpu_t log_id) {
		assert(log_id < Hip::MAX_CPUS);
		return _cpus[log_id];
	}
	static size_t count() {
		return _count;
	}

	static iterator begin() {
		return SListIterator<CPU>(_online);
	}
	static iterator end() {
		return SListIterator<CPU>();
	}

	uint8_t flags;
	uint8_t thread;
	uint8_t core;
	uint8_t package;

	Pt *ds_pt;
	Pt *io_pt;
	Pt *gsi_pt;
	Pt *reg_pt;
	Pt *unreg_pt;
	Pt *get_pt;

	cpu_t phys_id() const {
		return _logtophys[_id];
	}
	cpu_t log_id() const {
		return _id;
	}

	CPU *next() {
		return _next;
	}

private:
	CPU() : ds_pt(), io_pt(), gsi_pt(), reg_pt(), unreg_pt(), get_pt(), _id(), _next() {
	}
	CPU(const CPU&);
	CPU& operator=(const CPU&);

	cpu_t _id;
	CPU *_next;
	static size_t _count;
	static CPU *_online;
	static CPU _cpus[Hip::MAX_CPUS];
	static cpu_t _logtophys[Hip::MAX_CPUS];
};

}
