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

#include <kobj/Pt.h>
#include <kobj/RichLocalEc.h>

namespace nul {

class RichPt : public Pt {
public:
	RichPt(RichLocalEc *ec,unsigned mtd) : Pt(ec,portal_wrapper,mtd) {
		ec->set_tls(Ec::TLS_PARAM,this);
		UtcbFrameRef uf(ec->utcb());
		ec->prepare_utcb(uf);
	}
	RichPt(RichLocalEc *ec,capsel_t pt,unsigned mtd) : Pt(ec,pt,portal_wrapper,mtd) {
		ec->set_tls(Ec::TLS_PARAM,this);
		UtcbFrameRef uf(ec->utcb());
		ec->prepare_utcb(uf);
	}
	virtual ~RichPt() {
		UtcbFrameRef uf;
		CapSpace::get().free(uf.get_receive_crd().cap(),1 << uf.get_receive_crd().order());
	}

protected:
	void finish_args(UtcbFrameRef &uf,bool refill_crd = false) {
		if(uf.has_more_untyped() || uf.has_more_typed())
			throw Exception(E_ARGS_INVALID);
		uf.clear();
		if(refill_crd)
			static_cast<RichLocalEc*>(Ec::current())->prepare_utcb(uf);
	}

private:
	virtual ErrorCode portal(UtcbFrameRef &uf,capsel_t pid) = 0;

	PORTAL static void portal_wrapper(capsel_t pid) {
		UtcbFrameRef uf;
		RichPt *pt = Ec::current()->get_tls<RichPt*>(Ec::TLS_PARAM);
		try {
			uf << pt->portal(uf,pid);
		}
		catch(const Exception &e) {
			Syscalls::revoke(uf.get_receive_crd(),true);
			uf.clear();
			uf << e.code();
		}
	}
};

}
