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

SessionData::SessionData(Service *s,size_t id,capsel_t pts,Pt::portal_func func)
	: RCUObject(), _id(id), _sm(), _caps(pts), _objs(), _ds() {
	for(uint i = 0; i < Hip::MAX_CPUS; ++i) {
		LocalEc *ec = s->get_ec(i);
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
