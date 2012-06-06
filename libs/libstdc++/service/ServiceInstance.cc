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

ServiceInstance::ServiceInstance(Service* s,capsel_t pt,cpu_t cpu)
		: _s(s), _ec(cpu), _pt(&_ec,pt,portal), _sm() {
	_ec.set_tls(_ec.create_tls(),s);
	// accept cap (for ds sharing)
	UtcbFrameRef ecuf(_ec.utcb());
	ecuf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
	ecuf.set_translate_crd(Crd(s->_caps,Util::nextpow2shift(Service::MAX_SESSIONS * Hip::MAX_CPUS),DESC_CAP_ALL));
}

void ServiceInstance::portal(capsel_t) {
	UtcbFrameRef uf;
	// TODO not everyone wants client-specific portals
	Service *s = Ec::current()->get_tls<Service>(0);
	try {
		Service::Command cmd;
		uf >> cmd;
		switch(cmd) {
			case Service::OPEN_SESSION: {
				SessionData *c = s->new_session();
				uf.clear();
				uf.delegate(CapRange(c->caps(),Hip::MAX_CPUS,DESC_CAP_ALL));
				uf << E_SUCCESS << s->_bf;
			}
			break;

			case Service::SHARE_DATASPACE: {
				TypedItem ti;
				uf.get_typed(ti);
				if(ti.crd().cap() == 0)
					throw Exception(E_ARGS_INVALID);

				SessionData *c = s->get_session<SessionData>(ti.crd().cap());
				c->_ds = new DataSpace();
				uf >> *c->_ds;
				c->_ds->map();

				uf.clear();
				uf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
				uf << E_SUCCESS;
			}
			break;
		}
	}
	catch(const Exception& e) {
		uf.clear();
		uf << e.code();
	}
}

}
