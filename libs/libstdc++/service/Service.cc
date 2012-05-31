/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <service/Service.h>

namespace nul {

void ClientData::portal_ds(capsel_t pid) {
	Service *s = Ec::current()->get_tls<Service>(0);
	ClientData *c = s->get_client<ClientData>(pid);
	UtcbFrameRef uf;
	c->_ds = new DataSpace();
	uf >> *c->_ds;
	c->_ds->map();
}

}
