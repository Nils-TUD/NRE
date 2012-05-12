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
#include <kobj/GlobalEc.h>
#include <kobj/Sc.h>
#include <kobj/Sm.h>
#include <CPU.h>

using namespace nul;

static void write();
extern "C" int start();

int start() {
	const Hip& hip = Hip::get();
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled())
			new Sc(new GlobalEc(write,it->id()),Qpd());
	}

	Sm sm(0);
	sm.down();
	return 0;
}

static void write() {
	Pt *screen = 0;
	UtcbFrame uf;
	uf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
	do {
		try {
			uf.clear();
			uf << String("screen");
			CPU::current().get_pt->call(uf);

			TypedItem initti;
			uf.get_typed(initti);
			{
				Pt init(initti.crd().cap());
				UtcbFrame uf;
				uf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
				init.call(uf);

				TypedItem ti;
				uf.get_typed(ti);
				screen = new Pt(ti.crd().cap());
			}
		}
		catch(const Exception& e) {
		}
	}
	while(screen == 0);

	uf.reset();
	while(1)
		screen->call(uf);
}
