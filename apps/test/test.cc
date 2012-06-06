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
#include <kobj/GlobalEc.h>
#include <kobj/Sc.h>
#include <kobj/Sm.h>
#include <service/Session.h>
#include <stream/OStringStream.h>
#include <stream/Serial.h>
#include <cap/Caps.h>
#include <Exception.h>

using namespace nul;

EXTERN_C void abort();

static Session *sess;
static DataSpace *ds;

static void verbose_terminate() {
	// TODO put that in abort or something?
	try {
		throw;
	}
	catch(const Exception& e) {
		e.write(Serial::get());
	}
	catch(...) {
		Serial::get().writef("Uncatched, unknown exception\n");
	}
	abort();
}

static void write(void *) {
	Pt &pt = sess->pt(CPU::current().id);
	int *data = reinterpret_cast<int*>(ds->virt());
	for(uint i = 0; i < 100; ++i) {
		UtcbFrame uf;
		*data = i;
		//uf << i;
		pt.call(uf);
	}
}

int main() {
	Caps::allocate(CapRange(0x3F8,6,Caps::TYPE_IO | Caps::IO_A));
	Serial::get().init();

	std::set_terminate(verbose_terminate);

	sess = new Session("screen");
	ds = new DataSpace(100,DataSpace::ANONYMOUS,DataSpace::RW);
	ds->map();
	ds->share(*sess);

	const Hip& hip = Hip::get();
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled())
			new Sc(new GlobalEc(write,it->id()),Qpd());
	}

	Sm sm(0);
	sm.down();
	return 0;
}
