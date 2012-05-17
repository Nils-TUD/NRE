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
PORTAL static void portal_startup(cap_t pid,void *tls);
PORTAL static void portal_map(cap_t pid,void *tls);
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

	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled()) {
			CPU &cpu = CPU::get(it->id());
			// TODO ?
			LocalEc *cpuec = new LocalEc(0,cpu.id);
			cpu.map_pt = new Pt(cpuec,portal_map);
			new Pt(cpuec,cpuec->event_base() + CapSpace::EV_STARTUP,portal_startup,MTD_RSP);
		}
	}

	Caps::allocate(CapRange(0x3F8,6,Caps::TYPE_IO | Caps::IO_A));
	Caps::allocate(CapRange(0xB8,Util::blockcount(80 * 25 * 2,ExecEnv::PAGE_SIZE),
			Caps::TYPE_MEM | Caps::MEM_RW));

	Serial::get().init();
	Screen::get().clear();
	std::set_terminate(verbose_terminate);

	Log::get().writef("SEL: %u, EXC: %u, VMI: %u, GSI: %u\n",
			hip.cfg_cap,hip.cfg_exc,hip.cfg_vm,hip.cfg_gsi);
	Log::get().writef("Memory:\n");
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		Log::get().writef("\taddr=%#Lx, size=%#Lx, type=%d\n",it->addr,it->size,it->type);
		if(it->type == Hip_mem::AVAILABLE) {
			Caps::allocate(CapRange(it->addr >> ExecEnv::PAGE_SHIFT,
					Util::blockcount(it->size,ExecEnv::PAGE_SIZE),Caps::TYPE_MEM | Caps::MEM_RWX,
					it->addr >> ExecEnv::PAGE_SHIFT));
		}
	}

	Log::get().writef("CPUs:\n");
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled()) {
			Log::get().writef("\tpackage=%u, core=%u, thread=%u, flags=%u\n",
					it->package,it->core,it->thread,it->flags);
		}
	}

	start_childs();

	{
		Sm sm(0);
		sm.down();
	}

	try {
		new Sc(new GlobalEc(mythread,0,0),Qpd());
		new Sc(new GlobalEc(mythread,0,1),Qpd(1,100));
		new Sc(new GlobalEc(mythread,0,1),Qpd(1,1000));
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
	for(Hip::mem_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		// we are the first one :)
		if(it->type == Hip_mem::MB_MODULE && i++ >= 1) {
			// map the memory of the module
			Caps::allocate(CapRange(it->addr >> ExecEnv::PAGE_SHIFT,it->size >> ExecEnv::PAGE_SHIFT,
					Caps::TYPE_MEM | Caps::MEM_RWX));
			// we assume that the cmdline does not cross pages
			if(it->aux)
				Caps::allocate(CapRange(it->aux >> ExecEnv::PAGE_SHIFT,1,Caps::TYPE_MEM | Caps::MEM_RW));
			if(it->aux) {
				// ensure that its terminated at the end of the page
				char *end = reinterpret_cast<char*>(Util::roundup<uintptr_t>(it->aux,ExecEnv::PAGE_SIZE) - 1);
				*end = '\0';
			}

			mng->load(it->addr,it->size,reinterpret_cast<char*>(it->aux));
		}
	}
}

static void portal_map(cap_t,void *) {
	UtcbFrameRef uf;
	CapRange range;
	uf >> range;
	uf.reset();
	uf.delegate(range,DelItem::FROM_HV);
}

static void portal_startup(cap_t,void *) {
	UtcbExcFrameRef uf;
	uf->mtd = MTD_RIP_LEN;
	uf->eip = *reinterpret_cast<uint32_t*>(uf->esp);
}

static void mythread(void *) {
	static UserSm sm;
	Ec *ec = Ec::current();
	while(1) {
		ScopedLock<UserSm> guard(&sm);
		Log::get().writef("I am Ec %u, running on CPU %u\n",ec->cap(),ec->cpu());
	}
}
