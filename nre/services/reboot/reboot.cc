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

#include <ipc/Service.h>
#include <utcb/UtcbFrame.h>
#include <CPU.h>

#include "HostReboot.h"

using namespace nre;

template<class T>
static void try_reboot(const char *name) {
	try {
		T method;
		method.reboot();
	}
	catch(const Exception &e) {
		Serial::get() << "Reboot via " << name << " failed: " << e.msg() << "\n";
	}
}

PORTAL static void portal_reboot(capsel_t) {
	UtcbFrameRef uf;

	// first try acpi, which should work on most systems
	try_reboot<HostRebootACPI>("ACPI");

	// if we're still here, try pci reset
	try_reboot<HostRebootPCIReset>("PCIReset");

	// try System Control Port A
	try_reboot<HostRebootSysCtrlPortA>("SysCtrlPortA");

	// finally try keyboard
	try_reboot<HostRebootKeyboard>("Keyboard");

	// hopefully not reached
	uf << E_FAILURE;
}

int main() {
	Service *srv = new Service("reboot", CPUSet(CPUSet::ALL), portal_reboot);
	srv->start();
	return 0;
}
