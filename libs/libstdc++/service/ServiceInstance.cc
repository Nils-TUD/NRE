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
		: _s(s), _ec(cpu,0), _pt(s,&_ec,pt), _sm() {
	// for session-identification
	UtcbFrameRef ecuf(_ec.utcb());
	ecuf.set_translate_crd(Crd(s->_caps,Math::next_pow2_shift(Service::MAX_SESSIONS * Hip::MAX_CPUS),DESC_CAP_ALL));
}

ErrorCode ServiceInstance::ServiceCapsPt::portal(UtcbFrameRef &uf,capsel_t) {
	Service::Command cmd;
	uf >> cmd;
	switch(cmd) {
		case Service::OPEN_SESSION: {
			finish_args(uf);

			SessionData *sess = _s->new_session();
			uf.delegate(CapRange(sess->caps(),Hip::MAX_CPUS,DESC_CAP_ALL));
		}
		break;

		case Service::SHARE_DATASPACE: {
			// the translated cap of the client identifies his session
			capsel_t sid = uf.get_translated().cap();
			ScopedPtr<DataSpace> ds(new DataSpace());
			uf >> *ds.get();
			finish_args(uf,true);

			{
				// ensure that just one dataspace can be shared with one session
				SessionData *sess = _s->get_session<SessionData>(sid);
				ScopedLock<UserSm> guard(&sess->sm());
				if(sess->ds())
					throw Exception(E_EXISTS);

				ds->map();
				try {
					sess->set_ds(ds.get());
					ds.release();
				}
				catch(...) {
					ds->unmap();
				}
			}
		}
		break;

		case Service::CLOSE_SESSION: {
			capsel_t sid = uf.get_translated().cap();
			finish_args(uf);

			_s->destroy_session(sid);
		}
		break;
	}
	return E_SUCCESS;
}

}
