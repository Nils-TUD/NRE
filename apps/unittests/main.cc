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
#include <cap/Caps.h>
#include <Syscalls.h>
#include <Hip.h>
#include <CPU.h>
#include <Math.h>
#include <Exception.h>
#include <cstring>
#include <Assert.h>

#include "tests/Pingpong.h"
#include "tests/UtcbTest.h"
#include "tests/RegionListTest.h"
#include "tests/DelegatePerf.h"
#include "tests/CatchEx.h"

using namespace nul;
using namespace nul::test;

EXTERN_C void abort();
PORTAL static void portal_map(capsel_t pid);
PORTAL static void portal_startup(capsel_t);

uchar _stack[ExecEnv::PAGE_SIZE] ALIGNED(ExecEnv::PAGE_SIZE);
static const TestCase testcases[] = {
	pingpong,
	catchex,
	delegateperf,
	utcbtest,
	regionlist
};

void verbose_terminate() {
	// TODO put that in abort or something?
	try {
		throw;
	}
	catch(const Exception& e) {
		Serial::get() << e;
	}
	catch(...) {
		Serial::get() << "Uncatched, unknown exception\n";
	}
	abort();
}

int main() {
	CPU &cpu = CPU::current();
	LocalEc *cpuec = new LocalEc(cpu.id);
	cpu.map_pt = new Pt(cpuec,portal_map);
	new Pt(cpuec,cpuec->event_base() + CapSpace::EV_STARTUP,portal_startup,Mtd(Mtd::RSP));

	// allocate serial ports and VGA memory
	Caps::allocate(CapRange(0x3F8,6,Crd::IO_ALL));
	Caps::allocate(CapRange(0xB9,Math::blockcount<size_t>(80 * 25 * 2,ExecEnv::PAGE_SIZE),
			Crd::MEM | Crd::RW,ExecEnv::PHYS_START_PAGE + 0xB9));

	Serial::get().init();
	Screen::get().clear();
	std::set_terminate(verbose_terminate);

	for(size_t i = 0; i < sizeof(testcases) / sizeof(testcases[0]); ++i) {
		Log::get().writef("Testing %s...\n",testcases[i].name);
		try {
			testcases[i].func();
		}
		catch(const Exception& e) {
			Log::get() << e;
		}
		Log::get().writef("Done\n");
	}
	return 0;
}

static void portal_map(capsel_t) {
	UtcbFrameRef uf;
	CapRange range;
	uf >> range;
	uf.reset();
	uf.delegate(range,UtcbFrame::FROM_HV);
}

static void portal_startup(capsel_t) {
	UtcbExcFrameRef uf;
	uf->mtd = Mtd::RIP_LEN;
	uf->rip = *reinterpret_cast<uint32_t*>(uf->rsp);
}
