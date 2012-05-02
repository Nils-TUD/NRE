/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <kobj/Pt.h>
#include <cap/CapSpace.h>

using namespace nul;

extern "C" int start();

PORTAL static void portal_write(cap_t);

int start() {
	Pt regpt(CapSpace::SRV_REGISTER);

	LocalEc ec(0);
	Pt writept(&ec,portal_write);

	UtcbFrame uf;
	uf << String("screen") << 0 << writept.cap();
	regpt.call(uf);
	return 0;
}

static void portal_write(cap_t) {

}
