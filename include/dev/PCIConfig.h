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
#include <utcb/UtcbFrame.h>
#include <Exception.h>
#include <CPU.h>

namespace nul {

class PCIConfig {
public:
	enum Command {
		READ,
		WRITE,
		ADDR,
		REBOOT
	};

private:
	PCIConfig();
};

class PCIConfigSession : public Session {
public:
	explicit PCIConfigSession(Connection &con) : Session(con), _pts(new Pt*[CPU::count()]) {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			_pts[cpu] = con.available_on(cpu) ? new Pt(caps() + cpu) : 0;
	}
	virtual ~PCIConfigSession() {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			delete _pts[cpu];
		delete[] _pts;
	}

	uint32_t read(uint32_t bdf,size_t offset) {
		UtcbFrame uf;
		uf << PCIConfig::READ << bdf << offset;
		_pts[CPU::current().log_id()]->call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
		uint32_t value;
		uf >> value;
		return value;
	}

	void write(uint32_t bdf,size_t offset,uint32_t value) {
		UtcbFrame uf;
		uf << PCIConfig::WRITE << bdf << offset << value;
		_pts[CPU::current().log_id()]->call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
	}

	uintptr_t addr(uint32_t bdf,size_t offset) {
		UtcbFrame uf;
		uf << PCIConfig::ADDR << bdf << offset;
		_pts[CPU::current().log_id()]->call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
		uintptr_t addr;
		uf >> addr;
		return addr;
	}

	void reboot() {
		UtcbFrame uf;
		uf << PCIConfig::REBOOT;
		_pts[CPU::current().log_id()]->call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
	}

private:
	Pt **_pts;
};

}
