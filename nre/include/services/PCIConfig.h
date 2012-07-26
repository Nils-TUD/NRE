/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <arch/Types.h>
#include <ipc/Connection.h>
#include <ipc/ClientSession.h>
#include <utcb/UtcbFrame.h>
#include <Exception.h>
#include <CPU.h>

namespace nre {

/**
 * Types for the PCI configuration service
 */
class PCIConfig {
public:
	/**
	 * The available commands
	 */
	enum Command {
		READ,
		WRITE,
		ADDR,
		REBOOT
	};

private:
	PCIConfig();
};

/**
 * Represents a session at the PCI configuration service
 */
class PCIConfigSession : public ClientSession {
public:
	/**
	 * Creates a new session with given connection
	 *
	 * @param con the connection
	 */
	explicit PCIConfigSession(Connection &con) : ClientSession(con), _pts(new Pt*[CPU::count()]) {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			_pts[cpu] = con.available_on(cpu) ? new Pt(caps() + cpu) : 0;
	}
	/**
	 * Destroys the session
	 */
	virtual ~PCIConfigSession() {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			delete _pts[cpu];
		delete[] _pts;
	}

	/**
	 * Reads a value from given bdf and offset
	 *
	 * @param bdf the bus-device-function triple
	 * @param offset the offset
	 * @return the value
	 */
	uint32_t read(uint32_t bdf,size_t offset) {
		UtcbFrame uf;
		uf << PCIConfig::READ << bdf << offset;
		_pts[CPU::current().log_id()]->call(uf);
		uf.check_reply();
		uint32_t value;
		uf >> value;
		return value;
	}

	/**
	 * Writes the given value to given bdf and offset
	 *
	 * @param bdf the bus-device-function triple
	 * @param offset the offset
	 * @param value the value to write
	 */
	void write(uint32_t bdf,size_t offset,uint32_t value) {
		UtcbFrame uf;
		uf << PCIConfig::WRITE << bdf << offset << value;
		_pts[CPU::current().log_id()]->call(uf);
		uf.check_reply();
	}

	/**
	 * Determines the address of given bdf and offset
	 *
	 * @param bdf the bus-device-function triple
	 * @param offset the offset
	 * @return the address
	 */
	uintptr_t addr(uint32_t bdf,size_t offset) {
		UtcbFrame uf;
		uf << PCIConfig::ADDR << bdf << offset;
		_pts[CPU::current().log_id()]->call(uf);
		uf.check_reply();
		uintptr_t addr;
		uf >> addr;
		return addr;
	}

	/**
	 * Tries to reboot the PCI via the PCI configuration space
	 */
	void reboot() {
		UtcbFrame uf;
		uf << PCIConfig::REBOOT;
		_pts[CPU::current().log_id()]->call(uf);
		uf.check_reply();
	}

private:
	Pt **_pts;
};

}
