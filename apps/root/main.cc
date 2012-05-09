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
#include <utcb/UtcbExc.h>
#include <utcb/Utcb.h>
#include <mem/RegionList.h>
#include <stream/Log.h>
#include <stream/Screen.h>
#include <Syscalls.h>
#include <String.h>
#include <Hip.h>
#include <CPU.h>
#include <exception>
#include <cstring>
#include <assert.h>

using namespace nul;

extern "C" void abort();
extern "C" int start();
static void map(const CapRange& range);
PORTAL static void portal_startup(cap_t pid);
PORTAL static void portal_map(cap_t pid);
static void mythread();
extern void start_childs();

uchar _stack[ExecEnv::PAGE_SIZE] ALIGNED(ExecEnv::PAGE_SIZE);
static Sm *sm;

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

int start() {
	Ec *ec = Ec::current();
	const Hip &hip = Hip::get();

	for(Hip::cpu_const_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled()) {
			CPU &cpu = CPU::get(it->id());
			// TODO ?
			LocalEc *cpuec = new LocalEc(cpu.id);
			cpu.map_pt = new Pt(cpuec,portal_map);
			new Pt(cpuec,cpuec->event_base() + CapSpace::EV_STARTUP,portal_startup,MTD_RSP);
		}
	}

	map(CapRange(0x3f8,6,DESC_IO_ALL));
	map(CapRange(0xB9,Util::blockcount(80 * 25 * 2,ExecEnv::PAGE_SIZE),DESC_MEM_ALL));

	Serial::get().init();
	Screen::get().clear();
	std::set_terminate(verbose_terminate);

	Log::get().writef("SEL: %u, EXC: %u, VMI: %u, GSI: %u\n",
			hip.cfg_cap,hip.cfg_exc,hip.cfg_vm,hip.cfg_gsi);
	Log::get().writef("Memory:\n");
	for(Hip::mem_const_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		Log::get().writef("\taddr=%#Lx, size=%#Lx, type=%d\n",it->addr,it->size,it->type);
		if(it->type == Hip_mem::AVAILABLE) {
			map(CapRange(it->addr >> ExecEnv::PAGE_SHIFT,
					Util::blockcount(it->size,ExecEnv::PAGE_SIZE),DESC_MEM_ALL,it->addr >> ExecEnv::PAGE_SHIFT));
		}
	}

	Log::get().writef("CPUs:\n");
	for(Hip::cpu_const_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled())
			Log::get().writef("\tpackage=%u, core=%u, thread=%u, flags=%u\n",it->package,it->core,it->thread,it->flags);
	}

	start_childs();

	while(1);

	/*RegionList l;
	l.add(0x1000,0x4000,0,RegionList::RW);
	l.add(0x8000,0x2000,0,RegionList::R);
	l.add(0x10000,0x8000,0,RegionList::RX);
	l.print(*log);
	l.add(0x2000,0x1000,0,RegionList::R);
	l.print(*log);
	l.remove(0x1000,0x8000);
	l.print(*log);
	l.remove(0x10000,0x4000);
	l.print(*log);*/

	try {
		sm = new Sm(1);

		new Sc(new GlobalEc(mythread,0),Qpd());
		new Sc(new GlobalEc(mythread,1),Qpd(1,100));
		new Sc(new GlobalEc(mythread,1),Qpd(1,1000));
	}
	catch(const SyscallException& e) {
		e.write(Log::get());
	}

	mythread();
	return 0;
}

static void map(const CapRange& range) {
	UtcbFrame uf;
	uf.set_receive_crd(Crd(0,31,range.attr()));
	uf << range;
	CPU::current().map_pt->call(uf);
}

static void portal_map(cap_t) {
	UtcbFrameRef uf;
	CapRange range;
	uf >> range;
	uf.reset();
	uf.delegate(range,DelItem::FROM_HV);
}

static void portal_startup(cap_t) {
	UtcbExcFrameRef uf;
	uf->mtd = MTD_RIP_LEN;
	uf->eip = *reinterpret_cast<uint32_t*>(uf->esp);
}

static void mythread() {
	Ec *ec = Ec::current();
	while(1) {
		sm->down();
		Log::get().writef("I am Ec %u, running on CPU %u\n",ec->cap(),ec->cpu());
		sm->up();
	}
}
