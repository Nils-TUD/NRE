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
#include <kobj/UserSm.h>
#include <cap/CapSpace.h>
#include <stream/Screen.h>
#include <stream/Serial.h>
#include <stream/Log.h>
#include <service/Service.h>
#include <service/ServiceInstance.h>
#include <ScopedLock.h>
#include <CPU.h>

using namespace nul;

extern "C" int start();
PORTAL static void portal_write(cap_t,void *);

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
	Serial::get().init();
	Log::get().writef("I am the screen service!!\n");

	Service s("screen",portal_write);
	const Hip& hip = Hip::get();
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled())
			s.reg(it->id());
	}
	s.wait();
	return 0;
}

static void portal_write(cap_t pid,void *) {
	static UserSm sm;
	ScopedLock<UserSm> guard(&sm);
	Serial::get().writef("Got a write-request on cpu %u from client %u\n",Ec::current()->cpu(),pid);
}
