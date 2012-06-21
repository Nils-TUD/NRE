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

#include <subsystem/Child.h>
#include <stream/OStream.h>

namespace nul {

OStream &operator<<(OStream &os,const Child &c) {
	os << "Child[cmdline='" << c.cmdline() << "', refs=" << c.refs() << ", entry=" << c.entry() << "]:\n";
	os << "\tGSIs: " << c.gsis() << "\n";
	os << c.reglist();
	os << "\tPorts:\n" << c.io();
	os << "\n";
	return os;
}

}
