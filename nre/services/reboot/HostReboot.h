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

#include <ipc/Connection.h>
#include <services/Keyboard.h>
#include <services/ACPI.h>
#include <services/PCIConfig.h>
#include <kobj/Ports.h>
#include <mem/DataSpace.h>

#include "HostRebootMethod.h"

class HostRebootKeyboard : public HostRebootMethod {
public:
	explicit HostRebootKeyboard();
	virtual void reboot();

private:
	nre::Connection _con;
	nre::KeyboardSession _sess;
};

class HostRebootFastGate : public HostRebootMethod {
public:
	explicit HostRebootFastGate();
	virtual void reboot();

private:
	nre::Ports _port;
};

class HostRebootPCIReset : public HostRebootMethod {
public:
	explicit HostRebootPCIReset();
	virtual void reboot();

private:
	nre::Connection _con;
	nre::PCIConfigSession _sess;
};

class HostRebootACPI : public HostRebootMethod {
public:
	explicit HostRebootACPI();
	virtual ~HostRebootACPI();
	virtual void reboot();

private:
	uint8_t _method;
	uint8_t _value;
	uint64_t _addr;
	nre::Connection _con;
	nre::ACPISession _sess;
	nre::Ports *_ports;
	nre::DataSpace *_ds;
};
