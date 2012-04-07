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

#include <cap/ResourceSpace.h>
#include <Syscalls.h>

namespace nul {

void ResourceSpace::allocate(uintptr_t base,size_t size,unsigned rights,uintptr_t target) {
	/*Utcb *utcb = Ec::current()->utcb();
	unsigned *item = utcb->msg;
	item[1] = target << 12 | MAP_HBIT;
	item[0] = Crd(base,size,_type | rights).value();
	utcb->head.untyped = 2;
	utcb->head.crd = Crd(0, 20, _type | rights).value();
	Syscalls::call(_pt);*/
}

}
