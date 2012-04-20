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
#include <Syscalls.h>
#include <String.h>
#include <Hip.h>
#include <Log.h>
#include <CPU.h>
#include <exception>
#include <assert.h>

using namespace nul;

extern "C" void abort();
extern "C" int start();
PORTAL static void portal_startup(unsigned pid);
PORTAL static void portal_test(unsigned pid);
PORTAL static void portal_map(unsigned pid);
static void mythread();

static Log *log;
static Sm *sm;

class A {
	int _x;

public:
	A() : _x() {}
	A(int x) : _x(x) {}

	void add() {
		_x++;
	}
	int get() const {
		return _x;
	}
};

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

class ElfException : public Exception {
public:
	ElfException(const char *msg) : Exception(msg) {
	}
};

template<typename T>
static T roundpow2(T val) {
	for(int i = sizeof(val) * 8 - 1; i >= 0; --i) {
		T bit = 1 << i;
		if(val & bit)
			return i;
	}
	return 0;
}

static void map_mem(uintptr_t phys,size_t size) {
	UtcbFrame uf;
	uintptr_t frame = phys >> ExecutionEnv::PAGE_SHIFT;
	size_t frames = size >> ExecutionEnv::PAGE_SHIFT;
	while(frames > 0) {
		size_t order = roundpow2(frames);
		uf << DelItem(Crd(frame,order,DESC_MEM_ALL),DelItem::FROM_HV,frame);
		uf.set_receive_crd(Crd(0, 31, DESC_MEM_ALL));
		CPU::current().map_pt->call();
		frame += 1 << order;
		frames -= 1 << order;
	}
}

static void load_mod(uintptr_t addr,size_t size) {
	ElfEh *elf = reinterpret_cast<ElfEh*>(addr);
	if(size < sizeof(ElfEh) || sizeof(ElfPh) > elf->e_phentsize || size < elf->e_phoff + elf->e_phentsize * elf->e_phnum)
		throw ElfException("Invalid ELF");
    if(!(elf->e_ident[0] == 0x7f && elf->e_ident[1] == 'E' && elf->e_ident[2] == 'L' && elf->e_ident[3] == 'F'))
		throw ElfException("Invalid signature");

    for(size_t i = 0; i < elf->e_phnum; i++) {
		ElfPh *ph = reinterpret_cast<ElfPh*>(addr + elf->e_phoff + i * elf->e_phentsize);
		if(ph->p_type != 1)
			continue;
		if(size < ph->p_offset + ph->p_filesz)
			throw ElfException("Load segment invalid");

		log->print("LOAD %zu: %p .. %p (%zu)\n",i,ph->p_vaddr,ph->p_vaddr + ph->p_memsz,ph->p_memsz);
	}
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
			new Pt(cpu.ec,0x1E,portal_startup,MTD_RSP);
		}
	}

	{
		UtcbFrame uf;
		uf << DelItem(Crd(0x3F8,3,DESC_IO_ALL),DelItem::FROM_HV);
		uf.set_receive_crd(Crd(0, 20, DESC_IO_ALL));
		CPU::current().map_pt->call();
	}

	log = new Log();
	std::set_terminate(verbose_terminate);

	{
		int x;
		x++;
		log->print("x=%d\n",x);
	}

	{
		A a;
		a.add();
		log->print("x=%d\n",a.get());
	}

	{
		UtcbFrame uf;
		uf << DelItem(Crd(0x100,4,DESC_MEM_ALL),DelItem::FROM_HV,0x200);
		uf.set_receive_crd(Crd(0, 31, DESC_MEM_ALL));
		CPU::current().map_pt->call();
	}

	*(char*)0x200000 = 4;
	*(char*)(0x200000 + ExecutionEnv::PAGE_SIZE * 16 - 1) = 4;
	assert(*(char*)0x200000 == 4);
	//assert(*(char*)0x200000 == 5);

	log->print("SEL: %u, EXC: %u, VMI: %u, GSI: %u\n",
			hip.cfg_cap,hip.cfg_exc,hip.cfg_vm,hip.cfg_gsi);
	log->print("Memory:\n");
	for(Hip::mem_const_iterator it = hip.mem_begin(); it != hip.mem_end(); ++it) {
		log->print("\taddr=%#Lx, size=%#Lx, type=%d\n",it->addr,it->size,it->type);
		if(it->type == Hip_mem::MB_MODULE) {
			map_mem(it->addr,it->size);
			load_mod(it->addr,it->size);
			while(1);
		}
	}

	log->print("CPUs:\n");
	for(Hip::cpu_const_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		if(it->enabled())
			log->print("\tpackage=%u, core=%u thread=%u, flags=%u\n",it->package,it->core,it->thread,it->flags);
	}

	{
		Pt pt(CPU::current().ec,portal_test);
		{
			UtcbFrame uf;
			uf << 4 << 344 << 0x1234;
			uf.print(*log);
			pt.call();
		}
		ec->utcb()->reset();
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
		CPU::current().map_pt->call();
	}

	while(1) {
		sm->down();
		log->print("I am Ec %u, running on CPU %u\n",Ec::current()->cap(),Ec::current()->cpu());
		sm->up();
	}
}
