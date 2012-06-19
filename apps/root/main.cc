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

#include <arch/Elf.h>
#include <kobj/GlobalEc.h>
#include <kobj/LocalEc.h>
#include <kobj/Sc.h>
#include <kobj/Sm.h>
#include <kobj/Pt.h>
#include <kobj/Gsi.h>
#include <kobj/Ports.h>
#include <utcb/UtcbFrame.h>
#include <utcb/Utcb.h>
#include <subsystem/ChildMemory.h>
#include <mem/RegionManager.h>
#include <mem/DataSpace.h>
#include <stream/Log.h>
#include <stream/Screen.h>
#include <subsystem/ChildManager.h>
#include <cap/Caps.h>
#include <Syscalls.h>
#include <String.h>
#include <Hip.h>
#include <CPU.h>
#include <Math.h>
#include <Exception.h>
#include <BitField.h>
#include <cstring>
#include <Assert.h>

#include "PhysicalMemory.h"
#include "VirtualMemory.h"
#include "Hypervisor.h"

using namespace nul;

EXTERN_C void dlmalloc_init();
PORTAL static void portal_pagefault(capsel_t);
PORTAL static void portal_startup(capsel_t pid);
static void start_childs();

// stack for initial Ec (overwrites the weak-symbol defined in Startup.cc)
uchar _stack[ExecEnv::PAGE_SIZE] ALIGNED(ExecEnv::PAGE_SIZE);
// stack for LocalEc of first CPU
static uchar ptstack[ExecEnv::PAGE_SIZE] ALIGNED(ExecEnv::PAGE_SIZE);
static ChildManager *mng;

// TODO clang!
// TODO KObjects reference-counted? copying, ...
// TODO the gcc_except_table aligns to 2MiB in the binary, so that they get > 2MiB large!?
// TODO what about resource-release when terminating entire subsystems?
// TODO it would be a good idea to protect us from stack over- or underflow
// TODO when a service dies, the client will notice it as soon as it tries to access the service
// again. then it will throw an exception and call abort(). this in turn will kill this Ec. but
// what if the client has more than one Ec? I mean, the client is basically dead and we should
// restart it (the service gets restarted as well).

int main() {
	const Hip &hip = Hip::get();

	// just init the current CPU to prevent that the startup-heap-size depends on the number of CPUs
	CPU &cpu = CPU::current();
	// use the local stack here since we can't map dataspaces yet
	LocalEc *ec = new LocalEc(cpu.id,ObjCap::INVALID,reinterpret_cast<uintptr_t>(ptstack));
	cpu.map_pt = new Pt(ec,PhysicalMemory::portal_map);
	cpu.unmap_pt = new Pt(ec,PhysicalMemory::portal_unmap);
	cpu.gsi_pt = new Pt(ec,Hypervisor::portal_gsi);
	cpu.io_pt = new Pt(ec,Hypervisor::portal_io);
	// accept translated caps
	UtcbFrameRef defuf(ec->utcb());
	defuf.accept_translates();
	new Pt(ec,ec->event_base() + CapSpace::EV_STARTUP,portal_startup,Mtd(Mtd::RSP));
	new Pt(ec,ec->event_base() + CapSpace::EV_PAGEFAULT,portal_pagefault,
			Mtd(Mtd::RSP | Mtd::GPR_BSD | Mtd::RIP_LEN | Mtd::QUAL));

	Hypervisor::init();
	PhysicalMemory::init();
	VirtualMemory::init();

	Serial::get().init(false);

	// add all available memory
	Serial::get().writef("SEL: %u, EXC: %u, VMI: %u, GSI: %u\n",
			hip.cfg_cap,hip.cfg_exc,hip.cfg_vm,hip.cfg_gsi);
	Serial::get().writef("Memory:\n");
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		Serial::get().writef("\taddr=%#Lx, size=%#Lx, type=%d, aux=%#x\n",it->addr,it->size,it->type,it->aux);
		// FIXME: why can't we use the memory above 4G?
		if(it->type == Hip_mem::AVAILABLE && it->addr < 0x100000000)
			PhysicalMemory::add(it->addr,it->size);
	}

	// remove all not available memory
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		if(it->type != Hip_mem::AVAILABLE)
			PhysicalMemory::remove(it->addr,Math::round_up<size_t>(it->size,ExecEnv::PAGE_SIZE));
	}

	// now allocate the available memory from the hypervisor
	PhysicalMemory::map_all();
	Serial::get() << "Virtual memory:\n" << VirtualMemory::regions();
	Serial::get() << "Physical memory:\n" << PhysicalMemory::regions();

	Serial::get().writef("CPUs:\n");
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		Serial::get().writef("\tpackage=%u, core=%u, thread=%u, flags=%u\n",
				it->package,it->core,it->thread,it->flags);
	}

	// now we can use dlmalloc (map-pt created and available memory added to pool)
	dlmalloc_init();

	// now init the stuff for all other CPUs (using dlmalloc)
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		if(it->id != CPU::current().id) {
			LocalEc *ec = new LocalEc(it->id);
			it->map_pt = new Pt(ec,PhysicalMemory::portal_map);
			it->unmap_pt = new Pt(ec,PhysicalMemory::portal_unmap);
			it->gsi_pt = new Pt(ec,Hypervisor::portal_gsi);
			it->io_pt = new Pt(ec,Hypervisor::portal_io);
			// accept translated caps
			UtcbFrameRef defuf(ec->utcb());
			defuf.accept_translates();
			new Pt(ec,ec->event_base() + CapSpace::EV_STARTUP,portal_startup,Mtd(Mtd::RSP));
			new Pt(ec,ec->event_base() + CapSpace::EV_PAGEFAULT,portal_pagefault,
					Mtd(Mtd::RSP | Mtd::GPR_BSD | Mtd::RIP_LEN | Mtd::QUAL));
		}
	}

	// free the io-ports again to make them usable for the log-service
	Hypervisor::release_ports(0x3f8,6);

	start_childs();

	{
		Sm sm(0);
		sm.down();
	}
	return 0;
}

static void start_childs() {
	mng = new ChildManager();
	int i = 0;
	const Hip &hip = Hip::get();
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		// we are the first one :)
		if(it->type == Hip_mem::MB_MODULE && i++ >= 1) {
			// map the memory of the module
			uintptr_t virt = VirtualMemory::alloc(it->size);
			Hypervisor::allocate_mem(it->addr,virt,it->size);

			// map command-line, if necessary
			char *aux = 0;
			if(it->aux) {
				// the cmdline might cross pages. so map one by one until the cmdline ends
				bool found_end = false;
				uintptr_t auxvirt = VirtualMemory::alloc(it->size);
				aux = reinterpret_cast<char*>(auxvirt + (it->aux & (ExecEnv::PAGE_SIZE - 1)));
				char *str = aux;
				for(size_t x = 0; !found_end; ++x) {
					Hypervisor::allocate_mem(it->aux,auxvirt,ExecEnv::PAGE_SIZE);
					while(reinterpret_cast<uintptr_t>(str) < auxvirt + ExecEnv::PAGE_SIZE) {
						if(!*str) {
							found_end = true;
							break;
						}
						str++;
					}
				}
			}

			Serial::get().writef("Loading module @ %p .. %p\n",virt,virt + it->size);
			mng->load(virt,it->size,aux);
		}
	}
}

static void portal_pagefault(capsel_t) {
	UtcbExcFrameRef uf;
	uintptr_t *addr,addrs[32];
	uintptr_t pfaddr = uf->qual[1];
	unsigned error = uf->qual[0];
	uintptr_t eip = uf->rip;

	Serial::get().writef("Root: Pagefault for %p @ %p on cpu %u, error=%#x\n",
			pfaddr,eip,CPU::current().id,error);
	ExecEnv::collect_backtrace(uf->rsp & ~(ExecEnv::PAGE_SIZE - 1),uf->rbp,addrs,32);
	Serial::get().writef("Backtrace:\n");
	addr = addrs;
	while(*addr != 0) {
		Serial::get().writef("\t%p\n",*addr);
		addr++;
	}

	// let the kernel kill us
	uf->rip = ExecEnv::KERNEL_START;
	uf->mtd = Mtd::RIP_LEN;
}

static void portal_startup(capsel_t) {
	UtcbExcFrameRef uf;
	uf->mtd = Mtd::RIP_LEN;
	uf->rip = *reinterpret_cast<word_t*>(uf->rsp + sizeof(word_t));
}
