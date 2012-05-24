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
#include <exception>
#include <cstring>
#include <assert.h>

using namespace nul;

extern "C" void abort();
PORTAL static void portal_startup(capsel_t pid);
PORTAL static void portal_hvmap(capsel_t);
static void mythread(void *tls);
static void start_childs();

uchar _stack[ExecEnv::PAGE_SIZE] ALIGNED(ExecEnv::PAGE_SIZE);

// TODO clang!
// TODO use dataspaces to pass around memory in an easy fashion? (and reference-counting)
// TODO perhaps we don't want to have a separate Pd for the logging service
// TODO but we want to have one for the console-stuff
// TODO perhaps we need a general concept for identifying clients
// TODO with the service system we have the problem that one client that uses a service on multiple
// CPUs is treaten as a different client
// TODO KObjects reference-counted? copying, ...
// TODO the gcc_except_table aligns to 2MiB in the binary, so that they get > 2MiB large!?

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

int main() {
	const Hip &hip = Hip::get();

	// create temporary map-portals to map stuff from HV
	LocalEc *ecs[Hip::MAX_CPUS];
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled()) {
			CPU &cpu = CPU::get(it->id());
			LocalEc *ec = new LocalEc(cpu.id);
			ecs[it->id()] = ec;
			cpu.map_pt = new Pt(ec,portal_hvmap);
			new Pt(ec,ec->event_base() + CapSpace::EV_STARTUP,portal_startup,MTD_RSP);
		}
	}

	// allocate serial ports and VGA memory
	Caps::allocate(CapRange(0x3F8,6,Caps::TYPE_IO | Caps::IO_A));
	Caps::allocate(CapRange(0xB9,Util::blockcount(80 * 25 * 2,ExecEnv::PAGE_SIZE),
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
			Caps::allocate(CapRange(start,pages,Caps::TYPE_MEM | Caps::MEM_RWX,addr));
			Memory::get().free(addr << ExecEnv::PAGE_SHIFT,pages << ExecEnv::PAGE_SHIFT);
		}
	}
	Memory::get().write(Log::get());
	Log::get().writef("CPUs:\n");
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled()) {
			Log::get().writef("\tpackage=%u, core=%u, thread=%u, flags=%u\n",
					it->package,it->core,it->thread,it->flags);
		}
	}

	{
		UtcbFrame uf;
		DataSpace ds;
		uf >> ds;
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
				throw Exception("Out of memory for modules");
			// map the memory of the module
			Caps::allocate(CapRange(it->addr >> ExecEnv::PAGE_SHIFT,it->size >> ExecEnv::PAGE_SHIFT,
					Caps::TYPE_MEM | Caps::MEM_RWX,start));
			uintptr_t addr = start << ExecEnv::PAGE_SHIFT;
			start += it->size >> ExecEnv::PAGE_SHIFT;
			// we assume that the cmdline does not cross pages
			char *aux = 0;
			if(it->aux) {
				if(((start + 1) << ExecEnv::PAGE_SHIFT) > ExecEnv::KERNEL_START)
					throw Exception("Out of memory for modules");
				Caps::allocate(CapRange(it->aux >> ExecEnv::PAGE_SHIFT,1,Caps::TYPE_MEM | Caps::MEM_RW,start));
				aux = reinterpret_cast<char*>((start << ExecEnv::PAGE_SHIFT) + (it->aux & (ExecEnv::PAGE_SIZE - 1)));
				start++;
				// ensure that its terminated at the end of the page
				uintptr_t endaddr = Util::roundup<uintptr_t>(reinterpret_cast<uintptr_t>(aux),ExecEnv::PAGE_SIZE) - 1;
				char *end = reinterpret_cast<char*>(endaddr);
				*end = '\0';
			}

			Log::get().writef("Loading module @ %p .. %p\n",addr,addr + it->size);
			mng->load(addr,it->size,aux);
		}
	}
}

static void portal_hvmap(capsel_t) {
	UtcbFrameRef uf;
	CapRange range;
	uf >> range;
	uf.clear();
	uf.delegate(range,DelItem::FROM_HV);
}

static void portal_startup(capsel_t) {
	UtcbExcFrameRef uf;
	uf->mtd = MTD_RIP_LEN;
	uf->rip = *reinterpret_cast<word_t*>(uf->rsp);
}

static void mythread(void *) {
	static UserSm sm;
	Ec *ec = Ec::current();
	while(1) {
		ScopedLock<UserSm> guard(&sm);
		Log::get().writef("I am Ec %u, running on CPU %u\n",ec->sel(),ec->cpu());
	}
}
