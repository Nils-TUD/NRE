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
#include <CPU.h>

#include "UtcbTest.h"

using namespace nul;
using namespace nul::test;

PORTAL static void portal_test(capsel_t);
static void test_nesting();
static void test_perf();

const TestCase utcbnest = {
	"Utcb nesting",test_nesting
};
const TestCase utcbperf = {
	"Utcb performance",test_perf
};
static const uint tries = 100000;
static uint64_t results[tries];

static void portal_test(capsel_t) {
	UtcbFrameRef uf;
	try {
		int a,b,c;
		uf >> a >> b >> c;
		uf.clear();
		uf << c << b << a;
	}
	catch(const Exception&) {
		uf.clear();
	}
}

static void test_nesting() {
	int a,b,c;
	LocalEc ec(CPU::current().log_id());
	Pt pt(&ec,portal_test);

	UtcbFrame uf;
	uf << 4 << 1 << 2;
	uf.delegate(10);
	uf.delegate(29);

	{
		UtcbFrame uf1;
		uf1 << 1 << 2 << 3;
		{
			UtcbFrame uf2;
			uf2 << 4 << 5 << 6;
			pt.call(uf2);

			uf2 >> a >> b >> c;
			WVPASSEQ(a,6);
			WVPASSEQ(b,5);
			WVPASSEQ(c,4);
		}
		WVPASSEQ(uf1.typed(),0UL);
		WVPASSEQ(uf1.untyped(),3UL);

		pt.call(uf1);

		uf1 >> a >> b >> c;
		WVPASSEQ(a,3);
		WVPASSEQ(b,2);
		WVPASSEQ(c,1);
	}
	WVPASSEQ(uf.typed(),2UL);
	WVPASSEQ(uf.untyped(),3UL);

	pt.call(uf);

	uf >> a >> b >> c;
	WVPASSEQ(a,2);
	WVPASSEQ(b,1);
	WVPASSEQ(c,4);
}

static void perform_test(size_t n,uint64_t rdtsc,uint64_t &min,uint64_t &max,uint64_t &avg) {
	uint64_t tic,tac,duration;
	min = ~0ull;
	max = 0;
	for(uint i = 0; i < tries; i++) {
		{
			UtcbFrame uf;
			for(size_t x = 0; x < n; ++x)
				uf << x;

			tic = Util::tsc();
			{
				UtcbFrame nested;
				nested << 1;
			}
			tac = Util::tsc();
		}
		duration = tac - tic - rdtsc;
		min = Math::min(min,duration);
		max = Math::max(max,duration);
		results[i] = duration;
	}
	avg = 0;
	for(uint i = 0; i < tries; i++)
		avg += results[i];
	avg = avg / tries;
}

static void test_perf() {
	uint64_t tic,tac,min,max,avg,rdtsc;
	tic = Util::tsc();
	tac = Util::tsc();
	rdtsc = tac - tic;
	size_t sizes[] = {1,2,4,8,16,32,64,128};
	for(size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
		perform_test(sizes[i],rdtsc,min,max,avg);
		WVPRINTF("Using %u words:",sizes[i]);
		WVPERF(avg,"cycles");
		WVPRINTF("avg: %Lu",avg);
		WVPRINTF("min: %Lu",min);
		WVPRINTF("max: %Lu",max);
	}
}
