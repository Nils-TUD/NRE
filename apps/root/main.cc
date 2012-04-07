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
#include <kobj/GlobalEc.h>
#include <kobj/LocalEc.h>
#include <kobj/Sc.h>
#include <kobj/Sm.h>
#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>
#include <utcb/UtcbExc.h>
#include <utcb/Utcb.h>
#include <Syscalls.h>
#include <String.h>
#include <Hip.h>
#include <Log.h>
#include <CPU.h>
#include <exception>
#include <assert.h>

#define PAGE_SIZE	0x1000

extern "C" void abort();
extern "C" int start(cpu_t cpu,Utcb *utcb);
PORTAL static void portal_startup(unsigned pid);
PORTAL static void portal_test(unsigned pid);
PORTAL static void portal_map(unsigned pid);
static void mythread();

static Log *log;
static Sm *sm;

void verbose_terminate() {
	try {
		throw;
	}
	catch(const Exception& e) {
		e.print(*log);
	}
	catch(...) {
		log->print("Uncatched, unknown exception\n");
	}
	abort();
}

int start(cpu_t cpu,Utcb *utcb) {
	const Hip &hip = Hip::get();
	GlobalEc::set_current(new GlobalEc(utcb,hip.cfg_exc + 1,cpu,new Pd(hip.cfg_exc + 0,true)));

	for(Hip::cpu_const_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled()) {
			CPU &cpu = CPU::get(it->id());
			cpu.id = it->id();
			cpu.ec = new LocalEc(cpu.id,hip.event_caps() * (cpu.id + 1));
			cpu.map_pt = new Pt(cpu.ec,portal_map);
			new Pt(cpu.ec,0x1E,portal_startup,MTD_RSP);
		}
	}

	{
		UtcbFrame uf;
		uf << DelItem(Crd(0x3F8,3,DESC_IO_ALL),DelItem::FROM_HV);
		uf.set_receive_crd(Crd(0, 20, DESC_IO_ALL));
		CPU::get(cpu).map_pt->call();
	}

	log = new Log();
	std::set_terminate(verbose_terminate);

	{
		UtcbFrame uf;
		uf << DelItem(Crd(0x100,4,DESC_MEM_ALL),DelItem::FROM_HV,0x200);
		uf.set_receive_crd(Crd(0, 31, DESC_MEM_ALL));
		CPU::get(cpu).map_pt->call();
	}

	*(char*)0x200000 = 4;
	*(char*)(0x200000 + PAGE_SIZE * 16 - 1) = 4;
	assert(*(char*)0x200000 == 4);
	//assert(*(char*)0x200000 == 5);

	log->print("SEL: %u, EXC: %u, VMI: %u, GSI: %u\n",
			hip.cfg_cap,hip.cfg_exc,hip.cfg_vm,hip.cfg_gsi);
	log->print("Memory:\n");
	for(Hip::mem_const_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		log->print("\taddr=%#Lx, size=%#Lx, type=%d\n",it->addr,it->size,it->type);
	}

	log->print("CPUs:\n");
	for(Hip::cpu_const_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled())
			log->print("\tpackage=%u, core=%u thread=%u, flags=%u\n",it->package,it->core,it->thread,it->flags);
	}

	{
		Pt pt(CPU::get(cpu).ec,portal_test);
		{
			UtcbFrame uf;
			uf << 4 << 344 << 0x1234;
			uf.print(*log);
			pt.call();
		}
		utcb->mtr = 0;
		const size_t NUM = 10000;
		static uint64_t calls[NUM];
		static uint64_t prepares[NUM];
		for(size_t j = 0; j < 10; j++) {
			for(size_t i = 0; i < NUM; i++) {
				uint64_t t1 = Util::tsc();
				UtcbFrame uf;
				uf << 4 << 344 << 0x1234;
				uint64_t t2 = Util::tsc();
				pt.call();
				uint64_t t3 = Util::tsc();
				prepares[i] = t2 - t1;
				calls[i] = t3 - t2;
			}
		}

		uint64_t calls_total = 0;
		uint64_t prepares_total = 0;
		for(size_t i = 0; i < NUM; i++) {
			calls_total += calls[i];
			prepares_total += prepares[i];
		}
		log->print("calls=%Lu : prepare=%Lu\n",calls_total / NUM,prepares_total / NUM);
	}

	while(1);

	try {
		sm = new Sm(1);

		new Sc(new GlobalEc(mythread,0),Qpd());
		new Sc(new GlobalEc(mythread,1),Qpd(1,100));
		new Sc(new GlobalEc(mythread,1),Qpd(1,1000));
	}
	catch(const SyscallException& e) {
		e.print(*log);
	}

	mythread();
	return 0;
}

static void portal_map(unsigned) {
	TypedItem ti;
	UtcbFrameRef uf;
	while(uf.has_more()) {
		uf >> ti;
		uf.add_typed(ti);
	}
}

static void portal_test(unsigned) {
	UtcbFrameRef uf;
	try {
		unsigned a,b,c;
		uf >> a >> b >> c;
	}
	catch(const Exception& e) {
		e.print(*log);
		uf.reset();
	}
}

static void portal_startup(unsigned) {
	UtcbExc *utcb = Ec::current()->exc_utcb();
	utcb->mtd = MTD_RIP_LEN;
	utcb->eip = *reinterpret_cast<uint32_t*>(utcb->esp);
}

static void mythread() {
	Ec *ec = Ec::current();
	{
		UtcbFrame uf;
		uf << DelItem(Crd(0x100 + ec->cap(),4,DESC_MEM_ALL),
				DelItem::FROM_HV,0x200 + ec->cap());
		uf.set_receive_crd(Crd(0, 31, DESC_MEM_ALL));
		CPU::get(ec->cpu()).map_pt->call();
	}

	while(1) {
		sm->down();
		log->print("I am Ec %u, running on CPU %u\n",Ec::current()->cap(),Ec::current()->cpu());
		sm->up();
	}
}
