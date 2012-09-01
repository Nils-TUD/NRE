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

#include <kobj/UserSm.h>
#include <kobj/Gsi.h>
#include <kobj/Ports.h>
#include <utcb/UtcbFrame.h>
#include <util/ScopedLock.h>
#include <Logging.h>

#include "Hypervisor.h"
#include "VirtualMemory.h"

using namespace nre;

uchar Hypervisor::_stack[ExecEnv::PAGE_SIZE] ALIGNED(ARCH_PAGE_SIZE);
Pt *Hypervisor::_map_pts[Hip::MAX_CPUS];
RegionManager Hypervisor::_io INIT_PRIO_HV;
BitField<Hip::MAX_GSIS> Hypervisor::_gsis INIT_PRIO_HV;
UserSm Hypervisor::_io_sm INIT_PRIO_HV;
UserSm Hypervisor::_gsi_sm INIT_PRIO_HV;
uint Hypervisor::_next_msi = 0;
Hypervisor Hypervisor::_init INIT_PRIO_HV;

Hypervisor::Hypervisor() {
	// note that we have to use a different Thread for the mem-portal than for all the other portals
	// in the root-task, because the map portal uses the mem-portal.
	uintptr_t ec_utcb = VirtualMemory::alloc(Utcb::SIZE);
	LocalThread *ec = new LocalThread(CPU::current().log_id(),ObjCap::INVALID,
			reinterpret_cast<uintptr_t>(_stack),ec_utcb);
	_map_pts[CPU::current().log_id()] = new Pt(ec,portal_map);
	// make all I/O ports available
	_io.free(0,0xFFFF);
}

void Hypervisor::init() {
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		if(it->log_id() == CPU::current().log_id())
			continue;
		LocalThread *ec = new LocalThread(it->log_id());
		_map_pts[it->log_id()] = new Pt(ec,portal_map);
	}
}

void Hypervisor::map_mem(uintptr_t phys,uintptr_t virt,size_t size) {
	UtcbFrame uf;
	uf.delegation_window(Crd(0,31,Crd::MEM_ALL));
	size_t pages = Math::blockcount<size_t>(size,ExecEnv::PAGE_SIZE);
	CapRange cr(phys >> ExecEnv::PAGE_SHIFT,pages,Crd::MEM_ALL,virt >> ExecEnv::PAGE_SHIFT);
	size_t count = pages;
	while(count > 0) {
		uf.clear();
		if(cr.hotspot() != CapRange::NO_HOTSPOT) {
			// if start and hotspot are different, we have to check whether it fits in the utcb
			uintptr_t diff = cr.hotspot() ^ cr.start();
			if(diff) {
				// the lowest bit that's different defines how many we can map with one Crd.
				// with bit 0, its 2^0 = 1 at once, with bit 1, 2^1 = 2 and so on.
				unsigned at_once = Math::bit_scan_forward(diff);
				if((1 << at_once) < count) {
					// be carefull that we might have to align it to 1 << at_once first. this takes
					// at most at_once typed items.
					size_t free = uf.free_typed();
					// we still have to put in the caprange
					free -= Math::blockcount(sizeof(CapRange),sizeof(word_t) * 2);
					size_t min = Math::min<uintptr_t>(free - at_once,count >> at_once);
					cr.count(min << at_once);
				}
			}
		}

		uf << cr;
		_map_pts[CPU::current().log_id()]->call(uf);

		// adjust start and hotspot for the next round
		cr.start(cr.start() + cr.count());
		if(cr.hotspot() != CapRange::NO_HOTSPOT)
			cr.hotspot(cr.hotspot() + cr.count());
		count -= cr.count();
		cr.count(count);
	}
}

char *Hypervisor::map_string(uintptr_t phys,uint max_pages) {
	if(!phys)
		return 0;

	// the cmdline might cross pages. so map one by one until the cmdline ends
	uintptr_t auxvirt = VirtualMemory::alloc(ExecEnv::PAGE_SIZE * max_pages);
	char *vaddr = reinterpret_cast<char*>(auxvirt + (phys & (ExecEnv::PAGE_SIZE - 1)));
	char *str = vaddr;
	phys &= ~(ExecEnv::PAGE_SIZE - 1);
	for(uint i = 0; i < max_pages; ++i) {
		map_mem(phys,auxvirt,ExecEnv::PAGE_SIZE);
		while(reinterpret_cast<uintptr_t>(str) < auxvirt + ExecEnv::PAGE_SIZE) {
			if(!*str)
				return vaddr;
			str++;
		}
		phys += ExecEnv::PAGE_SIZE;
		auxvirt += ExecEnv::PAGE_SIZE;
	}
	// ok, limit reached, so terminate it
	*reinterpret_cast<char*>(auxvirt - 1) = '\0';
	return vaddr;
}

void Hypervisor::portal_map(capsel_t) {
	UtcbFrameRef uf;
	CapRange range;
	uf >> range;
	uf.clear();
	uf.delegate(range,UtcbFrame::FROM_HV);
}

void Hypervisor::portal_gsi(capsel_t) {
	UtcbFrameRef uf;
	try {
		uint gsi;
		void *pcicfg = 0;
		Gsi::Op op;
		uf >> op >> gsi;
		if(op == Gsi::ALLOC)
			uf >> pcicfg;
		uf.finish_input();
		// we can trust the values because it's send from ourself
		assert(gsi < Hip::MAX_GSIS);

		switch(op) {
			case Gsi::ALLOC:
				LOG(Logging::RESOURCES,
						Serial::get().writef("Root: Allocating GSI %u (PCI %p)\n",gsi,pcicfg));
				allocate_gsi(gsi,pcicfg);
				break;

			case Gsi::RELEASE:
				LOG(Logging::RESOURCES,
						Serial::get().writef("Root: Releasing GSI %u\n",gsi));
				release_gsi(gsi);
				CapRange(Hip::MAX_CPUS + gsi,1,Crd::OBJ_ALL).revoke(false);
				break;
		}

		uf << E_SUCCESS;
		if(op == Gsi::ALLOC) {
			uf << gsi;
			uf.delegate(Hip::MAX_CPUS + gsi,0,UtcbFrame::FROM_HV);
		}
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e.code();
	}
}

void Hypervisor::portal_io(capsel_t) {
	UtcbFrameRef uf;
	try {
		Ports::port_t base;
		uint count;
		Ports::Op op;
		uf >> op >> base >> count;
		uf.finish_input();

		switch(op) {
			case Ports::ALLOC:
				LOG(Logging::RESOURCES,
						Serial::get().writef("Root: Allocating ports %#x..%#x\n",base,base + count - 1));
				allocate_ports(base,count);
				break;

			case Ports::RELEASE:
				LOG(Logging::RESOURCES,
						Serial::get().writef("Root: Releasing ports %#x..%#x\n",base,base + count - 1));
				release_ports(base,count);
				CapRange(base,count,Crd::IO_ALL).revoke(false);
				break;
		}

		if(op == Ports::ALLOC)
			uf.delegate(CapRange(base,count,Crd::IO_ALL),UtcbFrame::FROM_HV);
		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e.code();
	}
}
