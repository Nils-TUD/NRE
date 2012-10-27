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

#include "CatchEx.h"

using namespace nre;
using namespace nre::test;

static void test_catchex();

const TestCase catchex = {
	"Catch exception", test_catchex
};

static const unsigned tries = 1000;

static void test_catchex() {
	unsigned sum = 0;
	AvgProfiler prof(tries);
	UtcbFrame uf;
	for(unsigned i = 0; i < tries; i++) {
		prof.start();
		try {
			throw Exception(E_CAPACITY);
		}
		catch(const Exception& e) {
			sum++;
		}
		prof.stop();
	}

	WVPERF(prof.avg(), "cycles");
	WVPASSEQ(sum, tries);
	WVPRINTF("sum: %u", sum);
	WVPRINTF("min: %Lu", prof.min());
	WVPRINTF("max: %Lu", prof.max());
}
