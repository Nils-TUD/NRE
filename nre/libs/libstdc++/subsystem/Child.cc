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

#include <subsystem/Child.h>
#include <subsystem/ChildManager.h>
#include <stream/OStream.h>
#include <utcb/UtcbFrame.h>
#include <CPU.h>

namespace nre {

void Child::release_gsis() {
	UtcbFrame uf;
	for(uint i = 0; i < Hip::MAX_GSIS; ++i) {
		if(_gsis.is_set(i)) {
			uf << Gsi::RELEASE << i;
			CPU::current().gsi_pt().call(uf);
			uf.clear();
		}
	}
}

void Child::release_ports() {
	UtcbFrame uf;
	for(RegionManager::iterator it = _io.begin(); it != _io.end(); ++it) {
		if(it->size) {
			uf << Ports::RELEASE << it->addr << it->size;
			CPU::current().io_pt().call(uf);
			uf.clear();
		}
	}
}

void Child::release_scs() {
	UtcbFrame uf;
	for(SList<SchedEntity>::iterator it = _scs.begin(); it != _scs.end(); ) {
		uf << Sc::STOP;
		uf.translate(it->cap());
		CPU::current().sc_pt().call(uf);
		uf.clear();
		SList<SchedEntity>::iterator cur = it++;
		delete &*cur;
	}
}

void Child::release_regs() {
	ScopedLock<UserSm> guard(&_cm->_sm);
	for(ChildMemory::iterator it = _regs.begin(); it != _regs.end(); ++it) {
		DataSpaceDesc desc = it->desc();
		if(it->cap() != ObjCap::INVALID && desc.type() != DataSpaceDesc::VIRTUAL)
			_cm->_dsm.release(desc, it->cap());
	}
}

OStream & operator<<(OStream &os, const Child &c) {
	os << "Child[cmdline='" << c.cmdline() << "' cpu=" << c._ec->cpu();
	os << " entry=" << Format<uintptr_t>("%p", c.entry()) << "]:\n";
	os << "\tScs:\n";
	for(SList<Child::SchedEntity>::iterator it = c.scs().begin(); it != c.scs().end(); ++it)
		os << "\t\t" << it->name() << " on CPU " << CPU::get(it->cpu()).phys_id() << "\n";
	os << "\tGSIs: " << c.gsis() << "\n";
	os << "\tPorts:\n" << c.io();
	os << c.reglist();
	os << "\n";
	return os;
}

}
