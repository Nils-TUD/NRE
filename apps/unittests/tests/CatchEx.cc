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
#include "CatchEx.h"

using namespace nul;
using namespace nul::test;

static void test_catchex();

const TestCase catchex = {
	"Catch exception",test_catchex
};

static const unsigned tries = 1000;
static uint64_t results[tries];

static void test_catchex() {
	uint64_t tic,tac,min = ~0ull,max = 0,duration,rdtsc;
	unsigned sum = 0;
	tic = Util::tsc();
	tac = Util::tsc();
	rdtsc = tac - tic;
	UtcbFrame uf;
	for(unsigned i = 0; i < tries; i++) {
		tic = Util::tsc();
		try {
			throw Exception(E_CAPACITY);
		}
		catch(const Exception& e) {
			sum++;
		}
		tac = Util::tsc();
		duration = tac - tic - rdtsc;
		min = Util::min(min,duration);
		max = Util::max(max,duration);
		results[i] = duration;
	}
	uint64_t avg = 0;
	for(unsigned i = 0; i < tries; i++)
		avg += results[i];

	avg = avg / tries;
	WVPERF(avg,"cycles");
	WVPASSEQ(sum,tries);
	Log::get().writef("sum: %u\n",sum);
	Log::get().writef("avg: %Lu\n",avg);
	Log::get().writef("min: %Lu\n",min);
	Log::get().writef("max: %Lu\n",max);
}
