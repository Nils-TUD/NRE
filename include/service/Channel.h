/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <kobj/Pt.h>
#include <service/Session.h>
#include <cap/CapSpace.h>
#include <utcb/UtcbFrame.h>

namespace nul {

class Channel {
public:
	Channel(Session* sess) : _pt(get_portal(sess->pt())) {
	}

	Pt &pt() {
		return _pt;
	}

private:
	static capsel_t get_portal(Pt &pt) {
		UtcbFrame uf;
		uf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
		uf << Service::OPEN_SESSION;
		pt.call(uf);

		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);

		TypedItem ti;
		uf.get_typed(ti);
		return ti.crd().cap();
	}

	Channel(const Channel&);
	Channel& operator=(const Channel&);

	Pt _pt;
};

}
