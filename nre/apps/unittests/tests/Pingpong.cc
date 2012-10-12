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
#include <utcb/UtcbFrame.h>
#include <util/Profiler.h>
#include <CPU.h>

#include "Pingpong.h"

using namespace nre;
using namespace nre::test;

PORTAL static void portal_test(capsel_t);
static void test_pingpong();

const TestCase pingpong = {
	"PingPong",test_pingpong
};

static const uint tries = 10000;

static void portal_test(capsel_t) {
	UtcbFrameRef uf;
	try {
		uint a,b,c;
		uf >> a >> b >> c;
		uf.clear();
		uf << (a + b) << (a + b + c);
	}
	catch(const Exception&) {
		uf.clear();
	}
}

static void test_pingpong() {
	LocalThread *ec = LocalThread::create(CPU::current().log_id());
	Pt pt(ec,portal_test);
	AvgProfiler prof(tries);
	uint sum = 0;
	UtcbFrame uf;
	for(uint i = 0; i < tries; i++) {
		prof.start();
		uf << 1 << 2 << 3;
		pt.call(uf);
		uint x = 0,y = 0;
		uf >> x >> y;
		uf.clear();
		sum += x + y;
		prof.stop();
	}

	WVPERF(prof.avg(),"cycles");
	WVPASSEQ(sum,(1 + 2) * tries + (1 + 2 + 3) * tries);
	WVPRINTF("sum: %u",sum);
	WVPRINTF("min: %Lu",prof.min());
	WVPRINTF("max: %Lu",prof.max());
}
