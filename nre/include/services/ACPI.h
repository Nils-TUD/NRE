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
#include <services/PCIConfig.h>
#include <mem/DataSpace.h>
#include <utcb/UtcbFrame.h>
#include <Exception.h>
#include <CPU.h>

namespace nre {

/**
 * Types for the ACPI service
 */
class ACPI {
public:
	/**
	 * The available commands
	 */
	enum Command {
		GET_MEM,
		FIND_TABLE,
		GET_GSI,
	};

	/**
	 * Root system descriptor table
	 */
	struct RSDT {
		char signature[4];
		uint32_t length;
		uint8_t revision;
		uint8_t checksum;
		char oemId[6];
		char oemTableId[8];
		uint32_t oemRevision;
		char creatorId[4];
		uint32_t creatorRevision;
	} PACKED;

private:
	ACPI();
};

/**
 * Represents a session at the ACPI service
 */
class ACPISession : public ClientSession {
public:
	/**
	 * Creates a new session with given connection
	 *
	 * @param con the connection
	 */
	explicit ACPISession(Connection &con) : ClientSession(con), _ds(), _pts(new Pt*[CPU::count()]) {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			_pts[cpu] = con.available_on(cpu) ? new Pt(caps() + cpu) : 0;
		get_mem();
	}
	/**
	 * Destroys this session
	 */
	virtual ~ACPISession() {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			delete _pts[cpu];
		delete[] _pts;
		delete _ds;
	}

	/**
	 * Finds the ACPI table with given name
	 *
	 * @param name the name of the table
	 * @param instance the instance that is encountered (0 = the first one, 1 = the second, ...)
	 * @return the RSDT or 0 if not found
	 */
	ACPI::RSDT *find_table(const String &name,uint instance = 0) const {
		UtcbFrame uf;
		uf << ACPI::FIND_TABLE << name << instance;
		_pts[CPU::current().log_id()]->call(uf);

		uf.check_reply();
		uintptr_t offset;
		uf >> offset;
		return reinterpret_cast<ACPI::RSDT*>(offset == 0 ? 0 : _ds->virt() + offset);
	}

	/**
	 * Search for the GSI that is triggered by the given device, specified by <bdf>.
	 *
	 * @param bdf the bus-device-function triple of the device
	 * @param pin the IRQ pin of the device
	 * @param parent_bdf the bus-device-function triple of the parent device (e.g. bridge)
	 * @return the GSI
	 */
	uint get_gsi(PCIConfig::bdf_type bdf,uint8_t pin,PCIConfig::bdf_type parent_bdf) const {
		UtcbFrame uf;
		uf << ACPI::GET_GSI << bdf << pin << parent_bdf;
		_pts[CPU::current().log_id()]->call(uf);

		uf.check_reply();
		uint gsi;
		uf >> gsi;
		return gsi;
	}

private:
	void get_mem() {
		UtcbFrame uf;
		uf << ACPI::GET_MEM;
		_pts[CPU::current().log_id()]->call(uf);

		uf.check_reply();
		DataSpaceDesc desc;
		uf >> desc;
		_ds = new DataSpace(desc);
	}

	DataSpace *_ds;
	Pt **_pts;
};

}
