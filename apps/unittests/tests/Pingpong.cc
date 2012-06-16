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

#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>
#include <Util.h>
#include <CPU.h>
#include "Pingpong.h"

using namespace nul;
using namespace nul::test;

PORTAL static void portal_test(capsel_t);
static void test_pingpong();

const TestCase pingpong = {
	"PingPong",test_pingpong
};

static const unsigned tries = 10000;
static uint64_t results[tries];

static void portal_test(capsel_t) {
	UtcbFrameRef uf;
	try {
		unsigned a,b,c;
		uf >> a >> b >> c;
		uf.clear();
		uf << (a + b) << (a + b + c);
	}
	catch(const Exception&) {
		uf.clear();
	}
}

static void test_pingpong() {
	LocalEc ec(CPU::current().id);
	Pt pt(&ec,portal_test);
	uint64_t tic,tac,min = ~0ull,max = 0,ipc_duration,rdtsc;
	unsigned sum = 0;
	tic = Util::tsc();
	tac = Util::tsc();
	rdtsc = tac - tic;
	UtcbFrame uf;
	for(unsigned i = 0; i < tries; i++) {
		tic = Util::tsc();
		uf << 1 << 2 << 3;
		pt.call(uf);
		unsigned x = 0,y = 0;
		uf >> x >> y;
		uf.clear();
		sum += x + y;
		tac = Util::tsc();
		ipc_duration = tac - tic - rdtsc;
		min = Math::min(min,ipc_duration);
		max = Math::max(max,ipc_duration);
		results[i] = ipc_duration;
	}
	uint64_t avg = 0;
	for(unsigned i = 0; i < tries; i++)
		avg += results[i];

	avg = avg / tries;
	WVPERF(avg,"cycles");
	WVPASSEQ(sum,(1 + 2) * tries + (1 + 2 + 3) * tries);
	Log::get().writef("sum: %u\n",sum);
	Log::get().writef("avg: %Lu\n",avg);
	Log::get().writef("min: %Lu\n",min);
	Log::get().writef("max: %Lu\n",max);
}
