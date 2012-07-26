/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <ipc/ServiceCPUHandler.h>
#include <ipc/Service.h>
#include <ipc/ClientSession.h>
#include <utcb/UtcbFrame.h>
#include <stream/Serial.h>
#include <util/ScopedLock.h>
#include <util/ScopedPtr.h>

namespace nre {

ServiceCPUHandler::ServiceCPUHandler(Service* s,capsel_t pt,cpu_t cpu)
		: _s(s), _session_ec(cpu), _service_ec(cpu), _pt(&_service_ec,pt,portal), _sm() {
	_service_ec.set_tls<Service*>(Thread::TLS_PARAM,s);
	UtcbFrameRef ecuf(_service_ec.utcb());
	// for session-identification
	ecuf.accept_translates(s->_caps,Math::next_pow2_shift(Service::MAX_SESSIONS * CPU::count()));
}

void ServiceCPUHandler::portal(capsel_t) {
	UtcbFrameRef uf;
	Service *s = Thread::current()->get_tls<Service*>(Thread::TLS_PARAM);
	try {
		ClientSession::Command cmd;
		uf >> cmd;
		switch(cmd) {
			case ClientSession::OPEN: {
				uf.finish_input();

				ServiceSession *sess = s->new_session();
				uf.delegate(CapRange(sess->caps(),CPU::count(),Crd::OBJ_ALL));
			}
			break;

			case ClientSession::CLOSE: {
				capsel_t sid = uf.get_translated().offset();
				uf.finish_input();

				s->destroy_session(sid);
			}
			break;
		}
		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e.code();
	}
}

}
