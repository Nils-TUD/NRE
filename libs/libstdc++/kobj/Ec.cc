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

#include <kobj/Ec.h>
#include <Compiler.h>
#include <RCU.h>

namespace nul {

void Ec::create(Pd *pd,Syscalls::ECType type,void *sp) {
	ScopedCapSels scs;
	Syscalls::create_ec(scs.get(),utcb(),sp,_cpu,_event_base,type,pd->sel());
	if(pd == Pd::current()) {
		utcb()->init();
		RCU::announce(this);
	}
	sel(scs.release());
}

// slot 0 is reserved
size_t Ec::_tls_idx = 1;

}
