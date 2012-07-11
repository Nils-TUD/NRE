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

#include <arch/Defines.h>
#include <subsystem/ChildMemory.h>
#include <stream/OStream.h>

namespace nre {

OStream &operator<<(OStream &os,const ChildMemory &cm) {
	os << "\tDataspaces:\n";
	for(ChildMemory::iterator it = cm.begin(); it != cm.end(); ++it) {
		uint flags = it->desc().perm();
		os.writef(
			"\t\t%p .. %p (%#0"FMT_WORD_HEXLEN"x bytes) %c%c%c <- %p\n",it->desc().virt(),
				it->desc().virt() + it->desc().size(),it->desc().size(),
				(flags & ChildMemory::R) ? 'r' : '-',
				(flags & ChildMemory::W) ? 'w' : '-',
				(flags & ChildMemory::X) ? 'x' : '-',
				it->desc().origin()
		);
	}
	return os;
}

}
