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

#include <kobj/LocalEc.h>
#include <utcb/UtcbFrame.h>
#include <Util.h>

namespace nul {

class RichLocalEc : public LocalEc {
public:
	enum {
		NONE	= -1U
	};

	RichLocalEc(cpu_t cpu,uint order = NONE) : LocalEc(cpu), _order(order) {
		UtcbFrameRef uf(utcb());
		prepare_utcb(uf);
	}

	void prepare_utcb(UtcbFrameRef &uf) {
		if(_order != NONE) {
			capsel_t caps = CapSpace::get().allocate(1 << _order,1 << _order);
			uf.set_receive_crd(Crd(caps,_order,DESC_CAP_ALL));
		}
	}

private:
	uint _order;
};

}
