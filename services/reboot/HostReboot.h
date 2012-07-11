/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
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
