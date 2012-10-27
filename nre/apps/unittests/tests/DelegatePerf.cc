/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#include <kobj/Pt.h>
#include <kobj/Ports.h>
#include <utcb/UtcbFrame.h>
#include <util/Profiler.h>
#include <CPU.h>

#include "DelegatePerf.h"

using namespace nre;
using namespace nre::test;

PORTAL static void portal_test(capsel_t);
static void test_delegate();

const TestCase delegateperf = {
	"Delegate-performance", test_delegate
};

static const size_t tries = 1000;

static void portal_test(capsel_t) {
	UtcbFrameRef uf;
	uf.delegate(CapRange(0x100, 4, Crd::IO_ALL));
}

static void test_delegate() {
	Ports ports(0x100, 1 << 2);
	LocalThread *ec = LocalThread::create(CPU::current().log_id());
	Pt pt(ec, portal_test);
	AvgProfiler prof(tries);
	UtcbFrame uf;
	uf.delegation_window(Crd(0, 31, Crd::IO_ALL));
	for(size_t i = 0; i < tries; i++) {
		prof.start();
		pt.call(uf);
		uf.clear();
		prof.stop();
	}

	WVPERF(prof.avg(), "cycles");
	WVPRINTF("min: %Lu", prof.min());
	WVPRINTF("max: %Lu", prof.max());
}
