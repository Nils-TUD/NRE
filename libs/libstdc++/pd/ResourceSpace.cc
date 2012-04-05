/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <pd/ResourceSpace.h>
#include <Syscalls.h>

void ResourceSpace::allocate(uintptr_t base,size_t size,unsigned rights,uintptr_t target) {
	/*Utcb *utcb = Ec::current()->utcb();
	unsigned *item = utcb->msg;
	item[1] = target << 12 | MAP_HBIT;
	item[0] = Crd(base,size,_type | rights).value();
	utcb->head.untyped = 2;
	utcb->head.crd = Crd(0, 20, _type | rights).value();
	Syscalls::call(_pt);*/
}
