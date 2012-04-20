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

#include <kobj/KObject.h>
#include <kobj/Pd.h>
#include <ex/SyscallException.h>
#include <Syscalls.h>

namespace nul {

KObject::~KObject() {
	if(_cap != INVALID) {
		// the destructor shouldn't throw
		try {
			Syscalls::revoke(Crd(_cap,0,DESC_CAP_ALL),true);
		}
		catch(const SyscallException&) {
			// ignore it
		}
		CapSpace::get().free(_cap);
	}
}

}
