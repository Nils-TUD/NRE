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

using namespace nul;

extern "C" int start();

PORTAL static void portal_write(cap_t);

int start() {
	// TODO this should be somewhere else
	cpu_t no = Ec::current()->cpu();
	CPU &cpu = CPU::get(no);
	cpu.id = no;
	cpu.ec = new LocalEc(cpu.id,Hip::get().service_caps() * (cpu.id + 1));
	// TODO do we really need this portal everywhere? and what should it do, exactly?
	cpu.map_pt = new Pt(cpu.ec->event_base() + CapSpace::SRV_MAP);

	Pt regpt(CapSpace::SRV_REGISTER);

	LocalEc ec(0);
	Pt writept(&ec,portal_write);

	UtcbFrame uf;
	uf.delegate(writept.cap());
	uf << String("screen") << 0;
	regpt.call(uf);

	Sm sm(0);
	sm.down();
	return 0;
}

static void portal_write(cap_t) {

}
