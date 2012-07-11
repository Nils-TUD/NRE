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

#include <kobj/LocalThread.h>
#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>
#include <subsystem/ChildManager.h>
#include <ipc/Service.h>
#include <String.h>
#include <Hip.h>
#include <CPU.h>
#include <util/Math.h>
#include <Exception.h>
#include <Logging.h>
#include <cstring>
#include <Assert.h>

#include "PhysicalMemory.h"
#include "VirtualMemory.h"
#include "Hypervisor.h"
#include "Log.h"

using namespace nre;

class CPU0Init {
	CPU0Init();
	static CPU0Init init;
};

EXTERN_C void dlmalloc_init();
PORTAL static void portal_reg(capsel_t);
PORTAL static void portal_pagefault(capsel_t);
PORTAL static void portal_startup(capsel_t pid);
static void start_childs();

CPU0Init CPU0Init::init INIT_PRIO_CPU0;

// stack for initial Thread (overwrites the weak-symbol defined in Startup.cc)
uchar _stack[ExecEnv::PAGE_SIZE] ALIGNED(ExecEnv::PAGE_SIZE);
// stack for LocalThread of first CPU
static uchar ptstack[ExecEnv::PAGE_SIZE] ALIGNED(ExecEnv::PAGE_SIZE);
static uchar regptstack[ExecEnv::PAGE_SIZE] ALIGNED(ExecEnv::PAGE_SIZE);
static ChildManager *mng;

CPU0Init::CPU0Init() {
	// just init the current CPU to prevent that the startup-heap-size depends on the number of CPUs
	CPU &cpu = CPU::current();
	uintptr_t ec_utcb = VirtualMemory::alloc(Utcb::SIZE);
	// use the local stack here since we can't map dataspaces yet
	LocalThread *ec = new LocalThread(cpu.log_id(),ObjCap::INVALID,
			reinterpret_cast<uintptr_t>(ptstack),ec_utcb);
	cpu.ds_pt = new Pt(ec,PhysicalMemory::portal_dataspace);
	cpu.gsi_pt = new Pt(ec,Hypervisor::portal_gsi);
	cpu.io_pt = new Pt(ec,Hypervisor::portal_io);
	// accept translated caps
	UtcbFrameRef defuf(ec->utcb());
	defuf.accept_translates();
	new Pt(ec,ec->event_base() + CapSpace::EV_STARTUP,portal_startup,Mtd(Mtd::RSP));
	new Pt(ec,ec->event_base() + CapSpace::EV_PAGEFAULT,portal_pagefault,
			Mtd(Mtd::RSP | Mtd::GPR_BSD | Mtd::RIP_LEN | Mtd::QUAL));
	// register portal for the log service
	uintptr_t regec_utcb = VirtualMemory::alloc(Utcb::SIZE);
	LocalThread *regec = new LocalThread(cpu.log_id(),ObjCap::INVALID,
			reinterpret_cast<uintptr_t>(regptstack),regec_utcb);
	UtcbFrameRef reguf(regec->utcb());
	reguf.accept_delegates(Math::next_pow2_shift(CPU::count()));
	cpu.reg_pt = new Pt(regec,portal_reg);
}

// TODO clang!
// TODO KObjects reference-counted? copying, ...
// TODO what about resource-release when terminating entire subsystems?
// TODO when a service dies, the client will notice it as soon as it tries to access the service
// again. then it will throw an exception and call abort(). this in turn will kill this Thread. but
// what if the client has more than one Thread? I mean, the client is basically dead and we should
// restart it (the service gets restarted as well).
// TODO perhaps it makes sense to separate the virtualization-stuff from the rest by putting it in
// a separate library? or even for other things, so that we have a small, general library and some
// more advanced libs on top of that
// TODO what about different permissions for dataspaces? i.e. client has R, service has W
// TODO we have to think about mapping device-memory again. it might be necessary to let multiple
// Pds map one device (e.g. use acpi to find a device and map it).
// TODO novaboot in tools

int main() {
	const Hip &hip = Hip::get();

	// add all available memory
	LOG(Logging::RESOURCES,Serial::get().writef("SEL: %u, EXC: %u, VMI: %u, GSI: %u\n",
			hip.cfg_cap,hip.cfg_exc,hip.cfg_vm,hip.cfg_gsi));
	LOG(Logging::MEM_MAP,Serial::get().writef("Memory:\n"));
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		LOG(Logging::MEM_MAP,Serial::get().writef(
				"\taddr=%#Lx, size=%#Lx, type=%d, aux=%#x\n",it->addr,it->size,it->type,it->aux));
		// FIXME: why can't we use the memory above 4G?
		if(it->type == HipMem::AVAILABLE && it->addr < 0x100000000)
			PhysicalMemory::add(it->addr,it->size);
	}

	// remove all not available memory
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		// also remove the BIOS-area (make it available as device-memory)
		if(it->type != HipMem::AVAILABLE || it->addr == 0)
			PhysicalMemory::remove(it->addr,Math::round_up<size_t>(it->size,ExecEnv::PAGE_SIZE));
	}

	// now allocate the available memory from the hypervisor
	PhysicalMemory::map_all();
	LOG(Logging::MEM_MAP,Serial::get() << "Virtual memory:\n" << VirtualMemory::regions());
	LOG(Logging::MEM_MAP,Serial::get() << "Physical memory:\n" << PhysicalMemory::regions());

	LOG(Logging::CPUS,Serial::get().writef("CPUs:\n"));
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		LOG(Logging::CPUS,Serial::get().writef("\tpackage=%u, core=%u, thread=%u, flags=%u\n",
				it->package,it->core,it->thread,it->flags));
	}

	// now we can use dlmalloc (map-pt created and available memory added to pool)
	dlmalloc_init();

	// now init the stuff for all other CPUs (using dlmalloc)
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it) {
		if(it->log_id() != CPU::current().log_id()) {
			LocalThread *ec = new LocalThread(it->log_id());
			it->ds_pt = new Pt(ec,PhysicalMemory::portal_dataspace);
			it->gsi_pt = new Pt(ec,Hypervisor::portal_gsi);
			it->io_pt = new Pt(ec,Hypervisor::portal_io);
			// accept translated caps
			UtcbFrameRef defuf(ec->utcb());
			defuf.accept_translates();
			new Pt(ec,ec->event_base() + CapSpace::EV_STARTUP,portal_startup,Mtd(Mtd::RSP));
			new Pt(ec,ec->event_base() + CapSpace::EV_PAGEFAULT,portal_pagefault,
					Mtd(Mtd::RSP | Mtd::GPR_BSD | Mtd::RIP_LEN | Mtd::QUAL));
			// register portal for the log service
			LocalThread *regec = new LocalThread(it->log_id());
			UtcbFrameRef reguf(ec->utcb());
			reguf.accept_delegates(Math::next_pow2_shift(CPU::count()));
			it->reg_pt = new Pt(regec,portal_reg);
		}
	}

	mng = new ChildManager();
	Log::get().reg();
	start_childs();

	Sm sm(0);
	sm.down();
	return 0;
}

static void start_childs() {
	int i = 0;
	const Hip &hip = Hip::get();
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		// we are the first one :)
		if(it->type == HipMem::MB_MODULE && i++ >= 1) {
			// map the memory of the module
			uintptr_t virt = VirtualMemory::alloc(it->size);
			Hypervisor::map_mem(it->addr,virt,it->size);
			// map command-line, if necessary
			char *aux = Hypervisor::map_string(it->aux);
			mng->load(virt,it->size,aux);

			// TODO temporary. skip everything behind vancouver
			if(aux && strstr(aux,"vancouver"))
				break;
		}
	}
}

static void portal_reg(capsel_t) {
	UtcbFrameRef uf;
	try {
		String name;
		BitField<Hip::MAX_CPUS> available;
		capsel_t cap = uf.get_delegated(uf.delegation_window().order()).offset();
		uf >> name;
		uf >> available;
		uf.finish_input();

		mng->reg_service(0,cap,name,available);

		uf.accept_delegates();
		uf << E_SUCCESS;
	}
	catch(const Exception& e) {
		Syscalls::revoke(uf.delegation_window(),true);
		uf.clear();
		uf << e.code();
	}
}

static void portal_pagefault(capsel_t) {
	UtcbExcFrameRef uf;
	uintptr_t *addr,addrs[32];
	uintptr_t pfaddr = uf->qual[1];
	unsigned error = uf->qual[0];
	uintptr_t eip = uf->rip;

	Serial::get().writef("Root: Pagefault for %p @ %p on cpu %u, error=%#x\n",
			pfaddr,eip,CPU::current().phys_id(),error);
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
