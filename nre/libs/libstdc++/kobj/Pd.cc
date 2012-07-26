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

#include <arch/ExecEnv.h>
#include <arch/Startup.h>
#include <kobj/Pd.h>
#include <kobj/Thread.h>
#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>
#include <util/ScopedCapSels.h>
#include <CPU.h>

namespace nre {

Pd Pd::_cur INIT_PRIO_PD (CapSelSpace::INIT_PD);

// don't revoke our own Pd
Pd::Pd(capsel_t cap) : ObjCap(cap,ObjCap::KEEP_CAP_BIT) {
	if(_startup_info.child) {
		// grab our initial caps (pd, ec, sc) from parent
		UtcbFrame uf;
		uf.delegation_window(Crd(CapSelSpace::INIT_PD,2,Crd::OBJ_ALL));
		Pt initpt(_startup_info.cpu * Hip::get().service_caps() + CapSelSpace::SRV_INIT);
		initpt.call(uf);
	}
}

Pd *Pd::current() {
	return ExecEnv::get_current_pd();
}

Pd::Pd(Crd crd,Pd *pd) : ObjCap() {
	ScopedCapSels scs;
	Syscalls::create_pd(scs.get(),crd,pd->sel());
	sel(scs.release());
}

}
