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

#include <kobj/LocalEc.h>
#include <kobj/Pt.h>
#include <CPU.h>

using namespace nul;

extern "C" int start();

int start() {
	// TODO this should be somewhere else
	cpu_t no = Ec::current()->cpu();
	CPU &cpu = CPU::get(no);
	cpu.id = no;
	cpu.ec = new LocalEc(cpu.id,Hip::get().service_caps() * (cpu.id + 1));

	Pt *screen = 0;
	UtcbFrame uf;
	uf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
	Pt getpt(CapSpace::SRV_GET);
	do {
		try {
			uf.clear();
			uf << String("screen");
			getpt.call(uf);

			TypedItem it;
			uf.get_typed(it);
			screen = new Pt(it.crd().cap());
		}
		catch(const Exception& e) {

		}
	}
	while(screen == 0);

	uf.reset();
	screen->call(uf);
	while(1)
		;
	return 0;
}
