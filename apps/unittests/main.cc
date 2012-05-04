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
#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>
#include <stream/Log.h>
#include <stream/Screen.h>
#include <Syscalls.h>
#include <Hip.h>
#include <CPU.h>
#include <exception>
#include <cstring>
#include <assert.h>

#include "tests/Pingpong.h"
#include "tests/UtcbTest.h"
#include "tests/RegionListTest.h"
#include "tests/DelegatePerf.h"

using namespace nul;
using namespace nul::test;

extern "C" void abort();
extern "C" int start();
static void map(const CapRange& range);
PORTAL static void portal_map(cap_t pid);
PORTAL static void portal_startup(cap_t);

uchar _stack[ExecEnv::PAGE_SIZE] ALIGNED(ExecEnv::PAGE_SIZE);
static const TestCase testcases[] = {
	//pingpong,
	delegateperf,
	//utcbtest,
	//regionlist
};

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
			cpu.id = it->id();
			cpu.ec = new LocalEc(cpu.id,hip.service_caps() * (cpu.id + 1));
			cpu.map_pt = new Pt(cpu.ec,portal_map);
			new Pt(cpu.ec,cpu.ec->event_base() + 0x1E,portal_startup,MTD_RSP);
		}
	}

	map(CapRange(0x3f8,6,DESC_IO_ALL));
	map(CapRange(0xB9,Util::blockcount(80 * 25 * 2,ExecEnv::PAGE_SIZE),DESC_MEM_ALL));

	Serial::get().init();
	Screen::get().clear();
	std::set_terminate(verbose_terminate);

	for(size_t i = 0; i < sizeof(testcases) / sizeof(testcases[0]); ++i) {
		Log::get().writef("Testing %s...\n",testcases[i].name);
		try {
			testcases[i].func();
		}
		catch(const Exception& e) {
			e.write(Log::get());
		}
		Log::get().writef("Done\n");
	}
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
