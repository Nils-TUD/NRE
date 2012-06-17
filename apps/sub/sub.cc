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

#include <kobj/Pt.h>
#include <kobj/Sm.h>
#include <stream/OStringStream.h>
#include <stream/Serial.h>
#include <subsystem/ChildManager.h>
#include <cap/Caps.h>
#include <Exception.h>

using namespace nul;

EXTERN_C void abort();

static char prog[] = {
#include "../../test.dump"
};

static void verbose_terminate() {
	// TODO put that in abort or something?
	try {
		throw;
	}
	catch(const Exception& e) {
		Serial::get() << e;
	}
	catch(...) {
		Serial::get() << "Uncatched, unknown exception\n";
	}
	abort();
}

int main() {
	Caps::allocate(CapRange(0x3F8,6,Crd::IO_ALL));
	Serial::get().init();
	std::set_terminate(verbose_terminate);

	// don't put it on the stack :)
	ChildManager *cm = new ChildManager();
	cm->load(reinterpret_cast<uintptr_t>(prog),sizeof(prog),"sub-test");

	Sm sm(0);
	sm.down();
	return 0;
}
