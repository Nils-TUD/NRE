/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include "mem.h"

using namespace nul;

RootMemory::RootDataSpace RootMemory::RootDataSpace::_slots[MAX_SLOTS];
UserSm *RootMemory::_sm;
RegionManager RootMemory::_mem;
DataSpaceManager<RootMemory::RootDataSpace> RootMemory::_dsmng;

RootMemory::RootDataSpace::RootDataSpace(const DataSpaceDesc &desc)
		: _desc(desc), _sel(), _unmapsel() {
	_desc.size(Math::round_up<size_t>(_desc.size(),ExecEnv::PAGE_SIZE));
	_desc.phys(RootMemory::regions().alloc(_desc.size()));
	_desc.virt(_desc.phys());

	// create a map and unmap-cap
	CapHolder cap(2,2);
	// TODO if this throws we might leak one sem and the memory
	Syscalls::create_sm(cap.get() + 0,0,Pd::current()->sel());
	Syscalls::create_sm(cap.get() + 1,0,Pd::current()->sel());

	_sel = cap.get() + 0;
	_unmapsel = cap.get() + 1;
	cap.release();
}

RootMemory::RootDataSpace::RootDataSpace(const DataSpaceDesc &,capsel_t)
		: _desc(), _sel(), _unmapsel() {
	// if we want to join a dataspace that does not exist in the root-task, its always an error
	throw DataSpaceException(E_NOT_FOUND);
}

RootMemory::RootDataSpace::~RootDataSpace() {
	// release memory
	revoke_mem(_desc.phys(),_desc.size());
	RootMemory::regions().free(_desc.phys(),_desc.size());

	// revoke caps and free selectors
	Syscalls::revoke(Crd(_sel,0,Crd::OBJ_ALL),true);
	CapSpace::get().free(_sel);
	Syscalls::revoke(Crd(_unmapsel,0,Crd::OBJ_ALL),true);
	CapSpace::get().free(_unmapsel);
}

void *RootMemory::RootDataSpace::operator new (size_t) throw() {
	for(size_t i = 0; i < MAX_SLOTS; ++i) {
		if(_slots[i]._desc.size() == 0)
			return _slots + i;
	}
	return 0;
}

void RootMemory::RootDataSpace::operator delete (void *ptr) throw() {
	RootDataSpace *ds = static_cast<RootDataSpace*>(ptr);
	ds->_desc.size(0);
}

void RootMemory::RootDataSpace::revoke_mem(uintptr_t addr,size_t size) {
	size_t count = size >> ExecEnv::PAGE_SHIFT;
	uintptr_t start = addr >> ExecEnv::PAGE_SHIFT;
	while(count > 0) {
		uint minshift = Math::minshift(start,count);
		Syscalls::revoke(Crd(start,minshift,Crd::MEM_ALL),false);
		start += 1 << minshift;
		count -= 1 << minshift;
	}
}

void RootMemory::init() {
	_sm = new UserSm();
}

void RootMemory::portal_map(capsel_t) {
	UtcbFrameRef uf;
	try {
		capsel_t sel = 0;
		DataSpaceDesc desc;
		DataSpace::RequestType type;
		uf >> type >> desc;
		if(type == DataSpace::JOIN)
			sel = uf.get_translated(0).cap();
		uf.finish_input();

		{
			// TODO move the lock up to the RootDataSpace
			ScopedLock<UserSm> guard(_sm);
			const RootDataSpace &ds = type == DataSpace::JOIN ? _dsmng.join(desc,sel) : _dsmng.create(desc);

			if(type == DataSpace::CREATE) {
				uf.delegate(ds.sel(),0);
				uf.delegate(ds.unmapsel(),1);
			}
			else
				uf.delegate(ds.unmapsel());
			// pass back attributes so that the caller has the correct ones
			uf << E_SUCCESS << ds.desc();
		}
	}
	catch(const Exception& e) {
		uf.clear();
		uf << e.code();
	}
}

void RootMemory::portal_unmap(capsel_t) {
	UtcbFrameRef uf;
	try {
		DataSpaceDesc desc;
		DataSpace::RequestType type;
		capsel_t sel = uf.get_translated(0).cap();
		uf >> type >> desc;
		uf.finish_input();

		{
			ScopedLock<UserSm> guard(_sm);
			_dsmng.release(desc,sel);
		}
	}
	catch(...) {
		// ignore
	}
}
