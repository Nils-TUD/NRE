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
#include <cap/Caps.h>
#include <stream/Screen.h>
#include <stream/Serial.h>
#include <stream/Log.h>
#include <service/Service.h>
#include <service/ServiceInstance.h>
#include <ScopedLock.h>
#include <CPU.h>

using namespace nul;

PORTAL static void portal_write(capsel_t);

int main() {
	// TODO might be something else than 0x3f8
	Caps::allocate(CapRange(0x3F8,6,Caps::TYPE_IO | Caps::IO_A));
	Caps::allocate(CapRange(0xB9,Util::blockcount(80 * 25 * 2,ExecEnv::PAGE_SIZE),
			Caps::TYPE_MEM | Caps::MEM_RW,ExecEnv::PHYS_START_PAGE + 0xB9));

	/*UtcbFrame uf;
	uf.add_typed(XltItem(Crd(CPU::current().map_pt->sel(),0,Caps::TYPE_CAP | Caps::ALL)));
	CPU::current().reg_pt->call(uf);*/

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

static void portal_write(capsel_t pid) {
	static UserSm sm;
	ScopedLock<UserSm> guard(&sm);
	UtcbFrameRef uf;
	uint i;
	uf >> i;
	Screen::get().writef("Request on cpu %u from %u: %u\n",Ec::current()->cpu(),pid,i);
}
