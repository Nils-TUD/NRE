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

#include <mem/DataSpace.h>
#include <kobj/Pd.h>
#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>
#include <ipc/ClientSession.h>
#include <ipc/Service.h>
#include <stream/Serial.h>
#include <util/ScopedCapSels.h>
#include <Syscalls.h>
#include <CPU.h>

namespace nre {

void DataSpace::create(DataSpaceDesc &desc, capsel_t *sel, capsel_t *unmapsel) {
	UtcbFrame uf;
	// prepare for receiving map and unmap-cap
	ScopedCapSels caps(2, 2);
	uf.delegation_window(Crd(caps.get(), 1, Crd::OBJ_ALL));
	uf << CREATE << desc;
	CPU::current().ds_pt().call(uf);

	uf.check_reply();
	uf >> desc;
	if(sel)
		*sel = caps.get();
	if(unmapsel)
		*unmapsel = caps.get() + 1;
	caps.release();
}

void DataSpace::create() {
	assert(_sel == ObjCap::INVALID && _unmapsel == ObjCap::INVALID);
	create(_desc, &_sel, &_unmapsel);
	// if its locked memory, make sure that it is mapped for us
	if(_desc.type() == DataSpaceDesc::LOCKED)
		touch();
}

void DataSpace::switch_to(DataSpace &dest) {
	UtcbFrame uf;
	uf.translate(unmapsel());
	uf.translate(dest.unmapsel());
	uf << SWITCH_TO;
	CPU::current().ds_pt().call(uf);
	uf.check_reply();
	uintptr_t tmp = _desc.origin();
	_desc.origin(dest._desc.origin());
	dest._desc.origin(tmp);
}

void DataSpace::join() {
	assert(_sel != ObjCap::INVALID && _unmapsel == ObjCap::INVALID);
	UtcbFrame uf;

	// prepare for receiving unmap-cap
	ScopedCapSels umcap;
	uf.delegation_window(Crd(umcap.get(), 0, Crd::OBJ_ALL));
	uf.translate(_sel);
	uf << JOIN;
	CPU::current().ds_pt().call(uf);

	uf.check_reply();
	uf >> _desc;
	_unmapsel = umcap.release();
	if(_desc.type() == DataSpaceDesc::LOCKED)
		touch();
}

void DataSpace::destroy() {
	assert(_sel != ObjCap::INVALID && _unmapsel != ObjCap::INVALID);
	UtcbFrame uf;

	// don't do that in the root-task. we allocate all memory at the beginning and simply manage
	// the usage of it. therefore, we never revoke it.
	if(_startup_info.child) {
		// ensure that the range is unmapped from our address space. this might not happen immediatly
		// otherwise because the ds might still be in use by somebody else. thus, the parent won't
		// revoke the memory in this case. but the parent might try to reuse the addresses in our
		// address space
		CapRange(_desc.virt() >> ExecEnv::PAGE_SHIFT,
		         _desc.size() >> ExecEnv::PAGE_SHIFT, Crd::MEM_ALL).revoke(true);
	}

	uf.translate(_unmapsel);
	uf << DESTROY << _desc;
	CPU::current().ds_pt().call(uf);

	CapSelSpace::get().free(_unmapsel);
	CapSelSpace::get().free(_sel);
}

void DataSpace::touch() {
	uint *addr = reinterpret_cast<uint*>(_desc.virt());
	uint *end = reinterpret_cast<uint*>(_desc.virt() + _desc.size());
	while(addr < end) {
		UNUSED volatile uint fetch = *addr;
		addr += ExecEnv::PAGE_SIZE / sizeof(uint);
	}
}

}
