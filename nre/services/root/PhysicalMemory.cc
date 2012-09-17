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

#include <Logging.h>

#include "PhysicalMemory.h"
#include "VirtualMemory.h"
#include "Hypervisor.h"

using namespace nre;

PhysicalMemory::RootDataSpace *PhysicalMemory::RootDataSpace::_free = 0;
size_t PhysicalMemory::_totalsize = 0;
RegionManager PhysicalMemory::_mem INIT_PRIO_PMEM;
DataSpaceManager<PhysicalMemory::RootDataSpace> PhysicalMemory::_dsmng INIT_PRIO_PMEM;

PhysicalMemory::RootDataSpace::RootDataSpace(const DataSpaceDesc &desc)
		: _desc(desc), _sel(), _unmapsel(), _next() {
	// TODO we leak resources here if something throws
	_desc.size(Math::round_up<size_t>(_desc.size(),ExecEnv::PAGE_SIZE));
	uint flags = _desc.perm();
	if(_desc.phys() != 0) {
		_desc.phys(Math::round_dn<uintptr_t>(_desc.phys(),ExecEnv::PAGE_SIZE));
		if(!PhysicalMemory::can_map(_desc.phys(),_desc.size(),flags)) {
			throw DataSpaceException(E_ARGS_INVALID,64,"Unable to map physical memory %p..%p",
				_desc.phys(),_desc.phys() + _desc.size());
		}
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

PhysicalMemory::RootDataSpace::RootDataSpace(capsel_t pid)
		: _desc(), _sel(), _unmapsel(), _next() {
	// if we want to join a dataspace that does not exist in the root-task, its always an error
	throw DataSpaceException(E_NOT_FOUND,32,"Dataspace %u not found in root",pid);
}

PhysicalMemory::RootDataSpace::~RootDataSpace() {
	// release memory
	uint flags = _desc.perm();
	bool isdev = PhysicalMemory::can_map(_desc.phys(),_desc.size(),flags);
	if(!isdev) {
		revoke_mem(_desc.virt(),_desc.size(),false);
		PhysicalMemory::_mem.free(_desc.phys(),_desc.size());
	}
	else {
		revoke_mem(_desc.virt(),_desc.size(),true);
		VirtualMemory::free(_desc.virt(),_desc.size());
	}

	// revoke caps and free selectors
	Syscalls::revoke(Crd(_sel,0,Crd::OBJ_ALL),true);
	CapSelSpace::get().free(_sel);
	Syscalls::revoke(Crd(_unmapsel,0,Crd::OBJ_ALL),true);
	CapSelSpace::get().free(_unmapsel);
}

void *PhysicalMemory::RootDataSpace::operator new (size_t) throw() {
	RootDataSpace *res;
	if(_free == 0) {
		uintptr_t phys = PhysicalMemory::_mem.alloc(ExecEnv::PAGE_SIZE);
		uintptr_t virt = VirtualMemory::alloc(ExecEnv::PAGE_SIZE);
		Hypervisor::map_mem(phys,virt,ExecEnv::PAGE_SIZE);
		RootDataSpace *ds = reinterpret_cast<RootDataSpace*>(virt);
		for(size_t i = 0; i < ExecEnv::PAGE_SIZE / sizeof(RootDataSpace); ++i) {
			ds->_next = _free;
			_free = ds;
			ds++;
		}
	}
	res = _free;
	_free = _free->_next;
	return res;
}

void PhysicalMemory::RootDataSpace::operator delete (void *ptr) throw() {
	RootDataSpace *ds = static_cast<RootDataSpace*>(ptr);
	ds->_next = _free;
	_free = ds;
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
	_totalsize = _mem.total_size();
}

bool PhysicalMemory::can_map(uintptr_t phys,size_t size,uint &flags) {
	const Hip &hip = Hip::get();
	// check for overflow
	if(phys + size < phys)
		return false;
	// check if its a module
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		if(it->type == HipMem::MB_MODULE) {
			uintptr_t end = Math::round_up<size_t>(it->addr + it->size,ExecEnv::PAGE_SIZE);
			if(phys >= it->addr && phys + size <= end) {
				// don't give the user write-access here
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
		uf >> type;
		if(type == DataSpace::JOIN || type == DataSpace::DESTROY)
			sel = uf.get_translated(0).offset();
		if(type != DataSpace::JOIN)
			uf >> desc;
		uf.finish_input();

		switch(type) {
			case DataSpace::CREATE:
			case DataSpace::JOIN:
				if(type != DataSpace::JOIN && desc.type() == DataSpaceDesc::VIRTUAL) {
					uintptr_t addr = VirtualMemory::alloc(desc.size());
					desc = DataSpaceDesc(desc.size(),desc.type(),desc.perm(),0,addr);
					LOG(Logging::DATASPACES,Serial::get() << "Root: Allocated virtual ds " << desc << "\n");
					uf << E_SUCCESS << desc;
				}
				else {
					const RootDataSpace &ds = type == DataSpace::JOIN
							? _dsmng.join(sel)
							: _dsmng.create(desc);

					if(type == DataSpace::CREATE) {
						LOG(Logging::DATASPACES,Serial::get() << "Root: Created " << ds << "\n");
						uf.delegate(ds.sel(),0);
						uf.delegate(ds.unmapsel(),1);
					}
					else {
						LOG(Logging::DATASPACES,Serial::get() << "Root: Joined " << ds << "\n");
						uf.delegate(ds.unmapsel());
					}
					// pass back attributes so that the caller has the correct ones
					uf << E_SUCCESS << ds.desc();
				}
				break;

			case DataSpace::DESTROY:
				if(desc.type() == DataSpaceDesc::VIRTUAL) {
					LOG(Logging::DATASPACES,
							Serial::get() << "Root: Destroyed virtual ds: " << desc << "\n");
					VirtualMemory::free(desc.virt(),desc.size());
				}
				else {
					LOG(Logging::DATASPACES,
							Serial::get() << "Root: Destroyed ds " << sel << ": " << desc << "\n");
					_dsmng.release(desc,sel);
				}
				uf << E_SUCCESS;
				break;

			case DataSpace::SWITCH_TO:
				assert(false);
				break;
		}
	}
	catch(const Exception& e) {
		uf.clear();
		uf << e;
	}
}
