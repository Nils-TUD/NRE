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
#include <format/Log.h>
#include <format/Screen.h>
#include <Syscalls.h>
#include <Hip.h>
#include <CPU.h>
#include <exception>
#include <cstring>
#include <assert.h>

using namespace nul;

extern "C" void abort();
extern "C" int start();
PORTAL static void portal_startup(cap_t pid);
PORTAL static void portal_test(cap_t pid);
PORTAL static void portal_map(cap_t pid);
static void mythread();
extern void start_childs();

uchar _stack[ExecEnv::PAGE_SIZE] ALIGNED(ExecEnv::PAGE_SIZE);
Log *log;
Screen *scr;
static Sm *sm;

void verbose_terminate() {
	// TODO put that in abort or something?
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

static void map(uintptr_t start,size_t count,uint attr,uintptr_t hotspot = 0) {
	UtcbFrame uf;
	uf.set_receive_crd(Crd(0,31,attr));
	if(hotspot == 0)
		hotspot = start;
	while(count > 0) {
		uint minshift = Util::minshift(start,count);
		uf << DelItem(Crd(start,minshift,attr),DelItem::FROM_HV,hotspot);
		start += 1 << minshift;
		hotspot += 1 << minshift;
		count -= 1 << minshift;
	}
	CPU::current().map_pt->call(uf);
}

int start() {
	Ec *ec = Ec::current();
	cpu_t cpu = ec->cpu();
	const Hip &hip = Hip::get();

	for(Hip::cpu_const_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled()) {
			CPU &cpu = CPU::get(it->id());
			cpu.id = it->id();
			cpu.ec = new LocalEc(cpu.id,hip.event_caps() * (cpu.id + 1));
			cpu.map_pt = new Pt(cpu.ec,portal_map);
			new Pt(cpu.ec,cpu.ec->event_base() + 0x1E,portal_startup,MTD_RSP);
		}
	}

	map(0x3f8,6,DESC_IO_ALL);
	map(0xB9,Util::blockcount(80 * 25 * 2,ExecEnv::PAGE_SIZE),DESC_MEM_ALL);

	log = new Log();
	scr = new Screen();
	std::set_terminate(verbose_terminate);

	map(0x100,16,DESC_MEM_ALL,0x200);

	*(char*)0x200000 = 4;
	*(char*)(0x200000 + ExecEnv::PAGE_SIZE * 16 - 1) = 4;
	assert(*(char*)0x200000 == 4);
	//assert(*(char*)0x200000 == 5);

	log->print("SEL: %u, EXC: %u, VMI: %u, GSI: %u\n",
			hip.cfg_cap,hip.cfg_exc,hip.cfg_vm,hip.cfg_gsi);
	log->print("Memory:\n");
	for(Hip::mem_const_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it)
		log->print("\taddr=%#Lx, size=%#Lx, type=%d\n",it->addr,it->size,it->type);

	log->print("CPUs:\n");
	for(Hip::cpu_const_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled())
			log->print("\tpackage=%u, core=%u thread=%u, flags=%u\n",it->package,it->core,it->thread,it->flags);
	}

	//start_childs();

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

	{
		Pt pt(CPU::current().ec,portal_test);
		{
			UtcbFrame uf;
			uf << 4 << 344 << 0x1234;
			uf << 1ULL;
			uf.add_typed(DelItem(Crd(12,4,DESC_IO_ALL),0,10));
			uf.add_typed(DelItem(Crd(1,1,DESC_MEM_ALL),0x1,2));

			{
				UtcbFrame uf;
				uf << 1 << 2 << 3;
				{
					UtcbFrame uf;
					uf << 4 << 5 << 6;
					pt.call(uf);

					unsigned x,y;
					uf >> x >> y;
					log->print("x=%u, y=%u\n",x,y);
				}
				pt.call(uf);

				unsigned x,y;
				uf >> x >> y;
				log->print("x=%u, y=%u\n",x,y);
			}

			uf.print(*log);
			pt.call(uf);
			uf.print(*log);

			unsigned x,y;
			uf >> x >> y;
			log->print("x=%u, y=%u\n",x,y);
		}
		while(1);

		static const unsigned tries = 10000;
		static uint64_t results[tries];
		uint64_t tic,tac,min = ~0ull,max = 0,ipc_duration,rdtsc;
		unsigned sum = 0;
		tic = Util::tsc();
		tac = Util::tsc();
		rdtsc = tac - tic;
		UtcbFrame uf;
		for(unsigned i = 0; i < tries; i++) {
			tic = Util::tsc();
			uf << 1 << 2 << 3;
			pt.call();
			unsigned x = 0,y = 0;
			uf >> x >> y;
			uf.reset();
			sum += x + y;
			tac = Util::tsc();
			ipc_duration = tac - tic - rdtsc;
			min = Util::min(min,ipc_duration);
			max = Util::max(max,ipc_duration);
			results[i] = ipc_duration;
		}
		uint64_t avg = 0;
		for(unsigned i = 0; i < tries; i++)
			avg += results[i];

		avg = avg / tries;
		//WVPASSEQ(counter, 1000u);
		scr->print("sum: %u\n",sum);
		scr->print("avg: %Lu\n",avg);
		scr->print("min: %Lu\n",min);
		scr->print("max: %Lu\n",max);

		while(1);

		/*
		const size_t NUM = 1000;
		static uint64_t calls[NUM];
		static uint64_t prepares[NUM];
		for(size_t j = 0; j < 10; j++) {
			for(size_t i = 0; i < NUM; i++) {
				uint64_t t1 = Util::tsc();
				UtcbFrame uf;
				uf << 4 << 344 << 0x1234;
				uint64_t t2 = Util::tsc();
				pt.call(uf);
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
		log->print("calls=%Lu : prepare=%Lu\n",calls_total / NUM,prepares_total / NUM);*/
	}

	Pd *pd = new Pd();
	new GlobalEc(reinterpret_cast<GlobalEc::startup_func>(0),0,pd);

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

static void portal_map(cap_t) {
	TypedItem ti;
	UtcbFrameRef uf;
	while(uf.has_more()) {
		uf >> ti;
		uf.add_typed(ti);
	}
}

static void portal_test(cap_t) {
	UtcbFrameRef uf;
	if(uf.untyped() != 3) {
		uf.reset();
		return;
	}

	unsigned a,b,c;
	uf >> a >> b >> c;
	//log->print("Got a=%u, b=%u, c=%u\n",a,b,c);
	uf.reset();
	uf << (a + b) << (a + b + c);
}

static void portal_startup(cap_t) {
	UtcbExcFrameRef uf;
	uf->mtd = MTD_RIP_LEN;
	uf->eip = *reinterpret_cast<uint32_t*>(uf->esp);
}

static void mythread() {
	Ec *ec = Ec::current();
	map(0x100 + ec->cap(),16,DESC_MEM_ALL,0x200 + ec->cap());
	while(1) {
		sm->down();
		log->print("I am Ec %u, running on CPU %u\n",Ec::current()->cap(),Ec::current()->cpu());
		sm->up();
	}
}
