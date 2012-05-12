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

#pragma once

#include <utcb/UtcbFrame.h>
#include <cap/CapSpace.h>
#include <kobj/Pt.h>
#include <CPU.h>

namespace nul {

class Client {
public:
	Client(const char *service) : _pt() {
		// TODO busy-waiting is bad
		do {
			try {
				cap_t cap = connect(service);
				cap_t pt = get_portal(cap);
				_pt = new Pt(pt);
			}
			catch(const Exception& e) {
			}
		}
		while(_pt == 0);
	}
	~Client() {
		delete _pt;
	}

	Pt *pt() {
		return _pt;
	}

private:
	cap_t connect(const char *service) {
		// TODO we need a function to receive a cap, right?
		UtcbFrame uf;
		uf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
		uf.clear();
		uf << String(service);
		CPU::current().get_pt->call(uf);

		TypedItem ti;
		uf.get_typed(ti);
		return ti.crd().cap();
	}

	cap_t get_portal(cap_t cap) {
		Pt init(cap);
		UtcbFrame uf;
		uf.set_receive_crd(Crd(CapSpace::get().allocate(),0,DESC_CAP_ALL));
		init.call(uf);

		TypedItem ti;
		uf.get_typed(ti);
		return ti.crd().cap();
	}

	Pt *_pt;
};

}
