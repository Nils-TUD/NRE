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

#include <service/ServiceInstance.h>
#include <service/Service.h>
#include <utcb/UtcbFrame.h>
#include <stream/Serial.h>
#include <cap/Caps.h>
#include <ScopedLock.h>
#include <ScopedPtr.h>

namespace nul {

ServiceInstance::ServiceInstance(Service* s,cpu_t cpu)
		: _s(s), _ec(cpu), _pt(&_ec,portal_newclient), _sm() {
	_ec.set_tls(_ec.create_tls(),s);
	// accept cap (for ds sharing)
	UtcbFrameRef ecuf(_ec.utcb());
	ecuf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));

	UtcbFrame uf;
	uf.delegate(_pt.sel());
	uf << String(s->name()) << cpu;
	CPU::current().reg_pt->call(uf);
	ErrorCode res;
	uf >> res;
	if(res != E_SUCCESS)
		throw Exception(res);
}

void ServiceInstance::portal_newclient(capsel_t) {
	UtcbFrameRef uf;
	// TODO not everyone wants client-specific portals
	Service *s = Ec::current()->get_tls<Service>(0);
	uf.clear();
	try {
		ClientData *c = s->new_client();
		uf.delegate(CapRange(c->pt().sel(),2,Caps::TYPE_CAP | Caps::ALL));
		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		uf << e.code();
	}
}

}
