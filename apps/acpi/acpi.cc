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

#include <service/Service.h>
#include <dev/ACPI.h>

#include "HostACPI.h"

using namespace nul;

static HostACPI *hostacpi;

PORTAL static void portal_acpi(capsel_t) {
	UtcbFrameRef uf;
	try {
		ACPI::Command cmd;
		uf >> cmd;

		if(!hostacpi)
			throw Exception(E_NOT_FOUND);

		switch(cmd) {
			case ACPI::GET_MEM: {
				uf.finish_input();
				uf.delegate(hostacpi->mem().sel());
				uf << E_SUCCESS << hostacpi->mem().desc();
			}
			break;

			case ACPI::FIND_TABLE: {
				String name;
				uint instance;
				uf >> name >> instance;
				uf.finish_input();

				uintptr_t off = hostacpi->find(name.str(),instance);
				uf << E_SUCCESS << off;
			}
			break;
		}
	}
	catch(const Exception &e) {
		uf.clear();
		uf << e.code();
	}
}

int main() {
	try {
		hostacpi = new HostACPI();
	}
	catch(const Exception &e) {
		Serial::get() << e;
	}

	Service *srv = new Service("acpi",portal_acpi);
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
		srv->provide_on(it->log_id());
	srv->reg();

	Sm sm(0);
	sm.down();
	return 0;
}
