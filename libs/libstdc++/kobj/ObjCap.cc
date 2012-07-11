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

#include <kobj/ObjCap.h>
#include <kobj/Pd.h>
#include <cap/CapSpace.h>
#include <Syscalls.h>

namespace nre {

ObjCap::~ObjCap() {
	if(_sel != INVALID) {
		if(!(_sel & KEEP_CAP_BIT)) {
			// the destructor shouldn't throw
			try {
				Syscalls::revoke(Crd(sel(),0,Crd::OBJ_ALL),true);
			}
			catch(const SyscallException&) {
				// ignore it
			}
		}
		if(!(_sel & KEEP_SEL_BIT))
			CapSpace::get().free(_sel);
	}
}

}
