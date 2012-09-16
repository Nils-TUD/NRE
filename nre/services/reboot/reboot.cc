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

PORTAL static void portal_reboot(capsel_t) {
	UtcbFrameRef uf;
	try {
		// first try keyboard
		HostRebootKeyboard kb;
		kb.reboot();

		// if we're still here, try gate a20
		HostRebootA20 fg;
		fg.reboot();

		// try pci reset
		HostRebootPCIReset pci;
		pci.reboot();

		// finally, try acpi
		HostRebootACPI acpi;
		acpi.reboot();

		// hopefully not reached
		uf << E_FAILURE;
	}
	catch(const Exception &e) {
		uf.clear();
		uf << e;
	}
}

int main() {
	Service *srv = new Service("reboot",CPUSet(CPUSet::ALL),portal_reboot);
	srv->start();
	return 0;
}
