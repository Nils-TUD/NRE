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

#include <ipc/Service.h>

namespace nre {

SessionData::SessionData(Service *s,size_t id,capsel_t pts,Pt::portal_func func)
	: RCUObject(), _id(id), _caps(pts), _objs(new ObjCap*[CPU::count()]) {
	for(uint i = 0; i < CPU::count(); ++i) {
		LocalThread *ec = s->get_ec(i);
		_objs[i] = 0;
		if(ec) {
			// just use portals if the service wants to provide one. otherwise use a semaphore to
			// prevent the client from calling us
			if(func)
				_objs[i] = new Pt(ec,pts + i,func);
			else
				_objs[i] = new Sm(pts + i,0U);
		}
	}
}

}
