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

#include <arch/Startup.h>
#include <kobj/GlobalEc.h>
#include <kobj/Pd.h>
#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>

using namespace nul;

// is overwritten by the root-task; all others don't need it
WEAK void *_stack;

void _setup(bool child) {
	static Pd initpd(CapSpace::INIT_PD,true);
	static GlobalEc initec(_startup_info.utcb,CapSpace::INIT_EC,_startup_info.cpu,&initpd);

	if(child) {
		// grab our initial caps (pd, ec, sc) from parent
		UtcbFrame uf;
		uf.set_receive_crd(Crd(CapSpace::INIT_PD,2,DESC_CAP_ALL));
		Pt initpt(CapSpace::SRV_INIT);
		initpt.call(uf);
	}
}
