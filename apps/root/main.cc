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
#include <utcb/UtcbFrame.h>
#include <utcb/Utcb.h>
#include <mem/RegionList.h>
#include <mem/Memory.h>
#include <mem/DataSpace.h>
#include <stream/Log.h>
#include <stream/Screen.h>
#include <subsystem/ChildManager.h>
#include <cap/Caps.h>
#include <Syscalls.h>
#include <String.h>
#include <Hip.h>
#include <CPU.h>
#include <Exception.h>
#include <cstring>
#include <Assert.h>

using namespace nul;

EXTERN_C void abort();
EXTERN_C void dlmalloc_init();
PORTAL static void portal_map(capsel_t);
PORTAL static void portal_unmap(capsel_t);
PORTAL static void portal_hvmap(capsel_t);
PORTAL static void portal_pagefault(capsel_t);
PORTAL static void portal_startup(capsel_t pid);
static void mythread(void *tls);
static void start_childs();

// stack for initial Ec (overwrites the weak-symbol defined in Startup.cc)
uchar _stack[ExecEnv::PAGE_SIZE] ALIGNED(ExecEnv::PAGE_SIZE);
// stack for LocalEc of first CPU
static uchar ptstack[ExecEnv::PAGE_SIZE] ALIGNED(ExecEnv::PAGE_SIZE);
static Pt *hv_pt = 0;
static Memory mem;

// TODO clang!
// TODO use dataspaces to pass around memory in an easy fashion? (and reference-counting)
// TODO perhaps we don't want to have a separate Pd for the logging service
// TODO but we want to have one for the console-stuff
// TODO perhaps we need a general concept for identifying clients
// TODO with the service system we have the problem that one client that uses a service on multiple
// CPUs is treaten as a different client
// TODO KObjects reference-counted? copying, ...
// TODO the gcc_except_table aligns to 2MiB in the binary, so that they get > 2MiB large!?
// TODO access to the first page doesn't cause a kill
// TODO how can we explicitly terminate childs?
// TODO what about resource-release when terminating entire subsystems?
// TODO it would be a good idea to protect us from stack over- or underflow

void verbose_terminate() {
	// TODO put that in abort or something?
	try {
		throw;
	}
	catch(const Exception& e) {
		e.write(Log::get());
	}
	catch(...) {
		Log::get().writef("Uncatched, unknown exception\n");
	}
	abort();
}

static void allocate(const CapRange& caps) {
	UtcbFrame uf;
	uf.set_receive_crd(Crd(0,31,caps.attr()));
	CapRange cr = caps;
	size_t count = cr.count();
	while(count > 0) {
		uf.clear();
		if(cr.hotspot()) {
			// if start and hotspot are different, we have to check whether it fits in the utcb
			uintptr_t diff = cr.hotspot() ^ cr.start();
			if(diff) {
				// the lowest bit that's different defines how many we can map with one Crd.
				// with bit 0, its 2^0 = 1 at once, with bit 1, 2^1 = 2 and so on.
				unsigned at_once = Util::bsf(diff);
				if((1 << at_once) < count)
					cr.count(Util::min<uintptr_t>(uf.freewords() / 2,count >> at_once) << at_once);
			}
		}

		uf << cr;
		hv_pt->call(uf);

		// adjust start and hotspot for the next round
		cr.start(cr.start() + cr.count());
		if(cr.hotspot())
			cr.hotspot(cr.hotspot() + cr.count());
		count -= cr.count();
		cr.count(count);
	}
}

int main() {
	const Hip &hip = Hip::get();

	// just init the current CPU to prevent that the startup-heap-size depends on the number of CPUs
	CPU &cpu = CPU::current();
	// use the local stack here since we can't map dataspaces yet
	LocalEc *ec = new LocalEc(cpu.id,ObjCap::INVALID,reinterpret_cast<uintptr_t>(ptstack));
	cpu.map_pt = new Pt(ec,portal_map);
	cpu.unmap_pt = new Pt(ec,portal_unmap);
	// accept translated caps
	UtcbFrameRef defuf(ec->utcb());
	defuf.set_translate_crd(Crd(0,31,DESC_CAP_ALL));
	// create the portal for allocating resources from HV for the current cpu
	hv_pt = new Pt(ec,portal_hvmap);
	new Pt(ec,ec->event_base() + CapSpace::EV_STARTUP,portal_startup,MTD_RSP);
	new Pt(ec,ec->event_base() + CapSpace::EV_PAGEFAULT,portal_pagefault,
			MTD_RSP | MTD_GPR_BSD | MTD_RIP_LEN | MTD_QUAL);

	// allocate serial ports and VGA memory
	allocate(CapRange(0x3F8,6,Caps::TYPE_IO | Caps::IO_A));
	allocate(CapRange(0xB9,Util::blockcount(80 * 25 * 2,ExecEnv::PAGE_SIZE),
			Caps::TYPE_MEM | Caps::MEM_RW,ExecEnv::PHYS_START_PAGE + 0xB9));

	Serial::get().init();
	Screen::get().clear();
	std::set_terminate(verbose_terminate);

	// map all available memory
	Log::get().writef("SEL: %u, EXC: %u, VMI: %u, GSI: %u\n",
			hip.cfg_cap,hip.cfg_exc,hip.cfg_vm,hip.cfg_gsi);
	Log::get().writef("Memory:\n");
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		Log::get().writef("\taddr=%#Lx, size=%#Lx, type=%d, aux=%#x\n",it->addr,it->size,it->type,it->aux);
		if(it->type == Hip_mem::AVAILABLE) {
			// map it to our physical memory area, but take care that it fits in there
			uintptr_t start = it->addr >> ExecEnv::PAGE_SHIFT;
			uintptr_t addr = start + ExecEnv::PHYS_START_PAGE;
			uintptr_t end = ExecEnv::MOD_START >> ExecEnv::PAGE_SHIFT;
			size_t pages = Util::blockcount(it->size,ExecEnv::PAGE_SIZE);
			if(addr >= end)
				continue;
			if(addr + pages > end)
				pages = end - addr;
			allocate(CapRange(start,pages,Caps::TYPE_MEM | Caps::MEM_RWX,addr));
			mem.free(addr << ExecEnv::PAGE_SHIFT,pages << ExecEnv::PAGE_SHIFT);
		}
	}
	mem.write(Log::get());
	Log::get().writef("CPUs:\n");
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled()) {
			Log::get().writef("\tpackage=%u, core=%u, thread=%u, flags=%u\n",
					it->package,it->core,it->thread,it->flags);
		}
	}

	// now we can use dlmalloc (map-pt created and available memory added to pool)
	dlmalloc_init();

	// now init the stuff for all other CPUs (using dlmalloc)
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled() && it->id() != CPU::current().id) {
			CPU &cpu = CPU::get(it->id());
			LocalEc *ec = new LocalEc(cpu.id);
			cpu.map_pt = new Pt(ec,portal_map);
			cpu.unmap_pt = new Pt(ec,portal_unmap);
			// accept translated caps
			UtcbFrameRef defuf(ec->utcb());
			defuf.set_translate_crd(Crd(0,31,DESC_CAP_ALL));
			new Pt(ec,ec->event_base() + CapSpace::EV_STARTUP,portal_startup,MTD_RSP);
			new Pt(ec,ec->event_base() + CapSpace::EV_PAGEFAULT,portal_pagefault,
					MTD_RSP | MTD_GPR_BSD | MTD_RIP_LEN | MTD_QUAL);
		}
	}

	start_childs();

	{
		Sm sm(0);
		sm.down();
	}

	try {
		new Sc(new GlobalEc(mythread,0),Qpd());
		new Sc(new GlobalEc(mythread,1),Qpd(1,100));
		new Sc(new GlobalEc(mythread,1),Qpd(1,1000));
	}
	catch(const SyscallException& e) {
		e.write(Log::get());
	}

	mythread(0);
	return 0;
}

static void start_childs() {
	ChildManager *mng = new ChildManager();
	int i = 0;
	const Hip &hip = Hip::get();
	uintptr_t start = ExecEnv::MOD_START_PAGE;
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		// we are the first one :)
		if(it->type == Hip_mem::MB_MODULE && i++ >= 1) {
			if((start << ExecEnv::PAGE_SHIFT) + it->size > ExecEnv::KERNEL_START)
				throw Exception(E_CAPACITY);
			// map the memory of the module
			allocate(CapRange(it->addr >> ExecEnv::PAGE_SHIFT,it->size >> ExecEnv::PAGE_SHIFT,
					Caps::TYPE_MEM | Caps::MEM_RWX,start));
			uintptr_t addr = start << ExecEnv::PAGE_SHIFT;
			start += it->size >> ExecEnv::PAGE_SHIFT;
			// TODO we assume that the cmdline does not cross pages
			char *aux = 0;
			if(it->aux) {
				if(((start + 1) << ExecEnv::PAGE_SHIFT) > ExecEnv::KERNEL_START)
					throw Exception(E_CAPACITY);
				allocate(CapRange(it->aux >> ExecEnv::PAGE_SHIFT,1,Caps::TYPE_MEM | Caps::MEM_RW,start));
				aux = reinterpret_cast<char*>(
						(start << ExecEnv::PAGE_SHIFT) + (it->aux & (ExecEnv::PAGE_SIZE - 1)));
				start++;
				// ensure that its terminated at the end of the page
				uintptr_t endaddr = Util::roundup<uintptr_t>(
						reinterpret_cast<uintptr_t>(aux),ExecEnv::PAGE_SIZE) - 1;
				char *end = reinterpret_cast<char*>(endaddr);
				*end = '\0';
			}

			Log::get().writef("Loading module @ %p .. %p\n",addr,addr + it->size);
			mng->load(addr,it->size,aux);
		}
	}
}

static void portal_map(capsel_t) {
	UtcbFrameRef uf;
	try {
		DataSpace ds;
		uf >> ds;
		// alloc mem
		size_t size = Util::roundup<size_t>(ds.size(),ExecEnv::PAGE_SIZE);
		uintptr_t addr = mem.alloc(size);
		// create a map and unmap-cap
		CapHolder cap(2,2);
		// TODO if this throws we might leak one sem and the memory
		Syscalls::create_sm(cap.get(),0,Pd::current()->sel());
		Syscalls::create_sm(cap.get() + 1,0,Pd::current()->sel());
		uf.clear();
		// delegate them to caller
		uf.delegate(cap.get(),0);
		uf.delegate(cap.get() + 1,1);
		// pass back attributes so that the caller has the correct ones
		uf << E_SUCCESS << addr << size << ds.perm() << ds.type();
		cap.release();
	}
	catch(const Exception& e) {
		uf.clear();
		uf << e.code();
	}
}

static void portal_unmap(capsel_t) {
	UtcbFrameRef uf;
	try {
		// free mem (we can trust the dataspace properties, because its sent from ourself)
		DataSpace ds;
		uf >> ds;
		mem.free(ds.virt(),ds.size());
		// revoke caps and free selectors
		Syscalls::revoke(Crd(ds.sel(),0,DESC_CAP_ALL),true);
		CapSpace::get().free(ds.sel());
		Syscalls::revoke(Crd(ds.unmapsel(),0,DESC_CAP_ALL),true);
		CapSpace::get().free(ds.unmapsel());
	}
	catch(...) {
		// ignore
	}
}

static void portal_hvmap(capsel_t) {
	UtcbFrameRef uf;
	CapRange range;
	uf >> range;
	uf.clear();
	uf.delegate(range,DelItem::FROM_HV);
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
	uf->mtd = MTD_RIP_LEN;
}

static void portal_startup(capsel_t) {
	UtcbExcFrameRef uf;
	uf->mtd = MTD_RIP_LEN;
	uf->rip = *reinterpret_cast<word_t*>(uf->rsp + sizeof(word_t));
}

static void mythread(void *) {
	static UserSm sm;
	Ec *ec = Ec::current();
	while(1) {
		ScopedLock<UserSm> guard(&sm);
		Log::get().writef("I am Ec %u, running on CPU %u\n",ec->sel(),ec->cpu());
	}
}
