/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include "PhysicalMemory.h"
#include "VirtualMemory.h"
#include "Hypervisor.h"

using namespace nul;

PhysicalMemory::RootDataSpace PhysicalMemory::RootDataSpace::_slots[MAX_SLOTS] INIT_PRIO_PMEM;
RegionManager PhysicalMemory::_mem INIT_PRIO_PMEM;
DataSpaceManager<PhysicalMemory::RootDataSpace> PhysicalMemory::_dsmng INIT_PRIO_PMEM;

PhysicalMemory::RootDataSpace::RootDataSpace(const DataSpaceDesc &desc)
		: _desc(desc), _sel(), _unmapsel() {
	// TODO we leak resources here if something throws
	_desc.size(Math::round_up<size_t>(_desc.size(),ExecEnv::PAGE_SIZE));
	uint flags = _desc.perm();
	if(_desc.phys() != 0) {
		_desc.phys(Math::round_dn<uintptr_t>(_desc.phys(),ExecEnv::PAGE_SIZE));
		if(!PhysicalMemory::can_map(_desc.phys(),_desc.size(),flags))
			throw DataSpaceException(E_ARGS_INVALID);
		_desc.perm(flags);
		_desc.virt(VirtualMemory::alloc(_desc.size()));
		_desc.origin(_desc.phys());
		Hypervisor::map_mem(_desc.phys(),_desc.virt(),_desc.size());
	}
	else {
		_desc.phys(PhysicalMemory::_mem.alloc(_desc.size()));
		_desc.origin(_desc.phys());
		_desc.virt(VirtualMemory::phys_to_virt(_desc.phys()));
	}

	// create a map and unmap-cap
	ScopedCapSels cap(2,2);
	Syscalls::create_sm(cap.get() + 0,0,Pd::current()->sel());
	Syscalls::create_sm(cap.get() + 1,0,Pd::current()->sel());

	_sel = cap.get() + 0;
	_unmapsel = cap.get() + 1;
	cap.release();
}

PhysicalMemory::RootDataSpace::RootDataSpace(const DataSpaceDesc &,capsel_t)
		: _desc(), _sel(), _unmapsel() {
	// if we want to join a dataspace that does not exist in the root-task, its always an error
	throw DataSpaceException(E_NOT_FOUND);
}

PhysicalMemory::RootDataSpace::~RootDataSpace() {
	// release memory
	uint flags = _desc.perm();
	bool isdev = PhysicalMemory::can_map(_desc.phys(),_desc.size(),flags);
	if(!isdev) {
		revoke_mem(_desc.virt(),_desc.size(),false);
		PhysicalMemory::_mem.free(_desc.phys(),_desc.size());
	}
	// TODO what to do with device memory?

	// revoke caps and free selectors
	Syscalls::revoke(Crd(_sel,0,Crd::OBJ_ALL),true);
	CapSpace::get().free(_sel);
	Syscalls::revoke(Crd(_unmapsel,0,Crd::OBJ_ALL),true);
	CapSpace::get().free(_unmapsel);
}

void *PhysicalMemory::RootDataSpace::operator new (size_t) throw() {
	for(size_t i = 0; i < MAX_SLOTS; ++i) {
		if(_slots[i]._desc.size() == 0)
			return _slots + i;
	}
	return 0;
}

void PhysicalMemory::RootDataSpace::operator delete (void *ptr) throw() {
	RootDataSpace *ds = static_cast<RootDataSpace*>(ptr);
	ds->_desc.size(0);
}

void PhysicalMemory::RootDataSpace::revoke_mem(uintptr_t addr,size_t size,bool self) {
	size_t count = size >> ExecEnv::PAGE_SHIFT;
	uintptr_t start = addr >> ExecEnv::PAGE_SHIFT;
	CapRange(start,count,Crd::MEM_ALL).revoke(self);
}

void PhysicalMemory::add(uintptr_t addr,size_t size) {
	if(VirtualMemory::alloc_ram(addr,size))
		_mem.free(addr,size);
}

void PhysicalMemory::remove(uintptr_t addr,size_t size) {
	_mem.remove(addr,size);
}

void PhysicalMemory::map_all() {
	for(RegionManager::iterator it = _mem.begin(); it != _mem.end(); ++it) {
		if(it->size)
			Hypervisor::map_mem(it->addr,VirtualMemory::phys_to_virt(it->addr),it->size);
	}
}

bool PhysicalMemory::can_map(uintptr_t phys,size_t size,uint &flags) {
	const Hip &hip = Hip::get();
	// check for overflow
	if(phys + size < phys)
		return false;
	// check if its a module
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		if(it->type == HipMem::MB_MODULE) {
			if(phys >= it->addr && phys + size <= Math::round_up<size_t>(it->addr + it->size,ExecEnv::PAGE_SIZE)) {
				// don't give the user write-access here
				flags = DataSpaceDesc::R;
				return true;
			}
			// TODO temporary. we need a better facility to access modules
			if(phys == Math::round_dn<uint64_t>(it->aux,ExecEnv::PAGE_SIZE)) {
				flags = DataSpaceDesc::R;
				return true;
			}
		}
	}
	// check if the child wants to request none-device-memory
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		if(!it->reserved() && Math::overlapped(phys,size,it->addr,it->size))
			return false;
	}
	return true;
}

void PhysicalMemory::portal_dataspace(capsel_t) {
	UtcbFrameRef uf;
	try {
		capsel_t sel = 0;
		DataSpaceDesc desc;
		DataSpace::RequestType type;
		uf >> type >> desc;
		if(type == DataSpace::JOIN || type == DataSpace::DESTROY)
			sel = uf.get_translated(0).offset();
		uf.finish_input();

		switch(type) {
			case DataSpace::CREATE:
			case DataSpace::JOIN:
				if(desc.type() == DataSpaceDesc::VIRTUAL) {
					uintptr_t addr = VirtualMemory::alloc(desc.size());
					uf << E_SUCCESS << DataSpaceDesc(desc.size(),desc.type(),desc.perm(),0,addr);
				}
				else {
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
				break;

			case DataSpace::DESTROY:
				if(desc.type() == DataSpaceDesc::VIRTUAL)
					VirtualMemory::free(desc.virt(),desc.size());
				else
					_dsmng.release(desc,sel);
				uf << E_SUCCESS;
				break;

			case DataSpace::SWITCH_TO:
			case DataSpace::SHARE:
				assert(false);
				break;
		}
	}
	catch(const Exception& e) {
		uf.clear();
		uf << e.code();
	}
}
