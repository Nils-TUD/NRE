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
	_ec.set_tls<Service*>(Ec::TLS_PARAM,s);
	UtcbFrameRef ecuf(_ec.utcb());
	// for dataspace sharing
	ecuf.accept_delegates(0);
	// for session-identification
	ecuf.accept_translates(s->_caps,Math::next_pow2_shift(Service::MAX_SESSIONS * Hip::MAX_CPUS));
}

void ServiceInstance::portal(capsel_t) {
	UtcbFrameRef uf;
	Service *s = Ec::current()->get_tls<Service*>(Ec::TLS_PARAM);
	try {
		Service::Command cmd;
		uf >> cmd;
		switch(cmd) {
			case Service::OPEN_SESSION: {
				uf.finish_input();

				SessionData *sess = s->new_session();
				uf.delegate(CapRange(sess->caps(),Hip::MAX_CPUS,Crd::OBJ_ALL));
			}
			break;

			case Service::SHARE_DATASPACE: {
				// the translated cap of the client identifies his session
				capsel_t sid = uf.get_translated().offset();
				capsel_t dssel = uf.get_delegated(0).offset();
				DataSpaceDesc desc;
				DataSpace::RequestType type;
				uf >> type >> desc;
				uf.finish_input();

				SessionData *sess = s->get_session<SessionData>(sid);
				ScopedPtr<DataSpace> ds(new DataSpace(desc,dssel));
				sess->accept_ds(ds.get());
				ds.release();

				uf.accept_delegates();
			}
			break;

			case Service::CLOSE_SESSION: {
				capsel_t sid = uf.get_translated().offset();
				uf.finish_input();

				s->destroy_session(sid);
			}
			break;
		}
		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.get_receive_crd(),true);
		uf.clear();
		uf << e.code();
	}
}

}
