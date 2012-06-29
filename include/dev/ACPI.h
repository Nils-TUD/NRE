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

#include <arch/Types.h>
#include <service/Connection.h>
#include <service/Session.h>
#include <mem/DataSpace.h>
#include <utcb/UtcbFrame.h>
#include <Exception.h>
#include <CPU.h>

namespace nul {

class ACPI {
public:
	enum Command {
		GET_MEM,
		FIND_TABLE
	};

private:
	ACPI();
};

class ACPISession : public Session {
public:
	explicit ACPISession(Connection &con) : Session(con), _ds(), _pts() {
		for(cpu_t cpu = 0; cpu < Hip::MAX_CPUS; ++cpu) {
			if(con.available_on(cpu))
				_pts[cpu] = new Pt(caps() + cpu);
		}
		get_mem();
	}
	virtual ~ACPISession() {
		for(cpu_t cpu = 0; cpu < Hip::MAX_CPUS; ++cpu)
			delete _pts[cpu];
		delete _ds;
	}

	uintptr_t find_table(const String &name,uint instance = 0) const {
		UtcbFrame uf;
		uf << ACPI::FIND_TABLE << name << instance;
		_pts[CPU::current().id]->call(uf);

		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
		uintptr_t offset;
		uf >> offset;
		return offset == 0 ? 0 : _ds->virt() + offset;
	}

private:
	void get_mem() {
		UtcbFrame uf;
		uf.accept_delegates(0);
		uf << ACPI::GET_MEM;
		_pts[CPU::current().id]->call(uf);

		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
		DataSpaceDesc desc;
		capsel_t sel = uf.get_delegated(0).offset();
		uf >> desc;
		_ds = new DataSpace(desc,sel);
	}

	DataSpace *_ds;
	Pt *_pts[Hip::MAX_CPUS];
};

}
