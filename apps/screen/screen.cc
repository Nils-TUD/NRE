/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <kobj/Pt.h>
#include <kobj/Sm.h>
#include <cap/CapSpace.h>
#include <stream/Screen.h>
#include <stream/Serial.h>
#include <CPU.h>

using namespace nul;

extern "C" int start();

PORTAL static void portal_write(cap_t);

int start() {
	{
		UtcbFrame uf;
		uf.set_receive_crd(Crd(0,31,DESC_MEM_ALL));
		uf << CapRange(0xB9,1,DESC_MEM_ALL);
		CPU::current().map_pt->call(uf);
	}
	{
		UtcbFrame uf;
		uf.set_receive_crd(Crd(0,31,DESC_IO_ALL));
		uf << CapRange(0x3F8,6,DESC_IO_ALL);
		CPU::current().map_pt->call(uf);
	}

	Screen::get().clear();
	Screen::get().writef("I am the screen service!!\n");
	Serial::get().init();
	Serial::get().writef("I am the screen service!!\n");

	const Hip& hip = Hip::get();
	for(Hip::cpu_const_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled()) {
			LocalEc *ec = new LocalEc(it->id());
			Pt *pt = new Pt(ec,portal_write);
			UtcbFrame uf;
			uf.delegate(pt->cap());
			uf << String("screen") << it->id();
			CPU::current().reg_pt->call(uf);
		}
	}

	Sm sm(0);
	sm.down();
	return 0;
}

static void portal_write(cap_t) {
	Serial::get().writef("Got a write-request on cpu %u\n",Ec::current()->cpu());
}
