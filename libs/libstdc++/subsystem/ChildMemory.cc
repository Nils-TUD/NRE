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

#include <subsystem/ChildMemory.h>
#include <stream/OStream.h>

namespace nre {

OStream &operator<<(OStream &os,const ChildMemory &cm) {
	os << "\tRegions:\n";
	for(size_t i = 0; i < ChildMemory::MAX_REGIONS; ++i) {
		const ChildMemory::Region *r = cm._regs + i;
		if(r->size > 0) {
			os.writef(
				"\t\t%zu: %p .. %p (%zu bytes) : %c%c%c%c, src=%p\n",i,r->begin,
					r->begin + r->size,r->size,
					(r->flags & ChildMemory::R) ? 'r' : '-',
					(r->flags & ChildMemory::W) ? 'w' : '-',
					(r->flags & ChildMemory::X) ? 'x' : '-',
					(r->flags & ChildMemory::M) ? 'm' : '-',
					r->src
			);
		}
	}
	os << "\tDataSpaces:\n";
	for(size_t i = 0; i < ChildMemory::MAX_DS; ++i) {
		if(cm._ds[i].unmapsel != 0) {
			const ChildMemory::DS *ds = cm._ds + i;
			os.writef("\t\t%u: %p .. %p (type %u, phys %p, org %p)\n",
					ds->unmapsel,ds->desc.virt(),ds->desc.virt() + ds->desc.size(),ds->desc.type(),
					ds->desc.phys(),ds->desc.origin());
		}
	}
	return os;
}

}
