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

SessionData::SessionData(Service *s,capsel_t pts,Pt::portal_func func)
	: _sm(), _caps(pts), _pts(), _ds() {
	for(uint i = 0; i < Hip::MAX_CPUS; ++i) {
		LocalEc *ec = s->get_ec(i);
		if(ec)
			_pts[i] = new Pt(ec,pts + i,func);
	}
}

}
