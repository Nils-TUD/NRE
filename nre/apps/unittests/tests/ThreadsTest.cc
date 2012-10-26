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

#include <kobj/GlobalThread.h>
#include <util/Profiler.h>
#include <CPU.h>

#include "ThreadsTest.h"

using namespace nre;
using namespace nre::test;

static void test_threads();

const TestCase threads = {
	"Threads",test_threads,
};
static const size_t TEST_COUNT = 50;

static void dummy(void*) {
}

static void test_threads() {
	static GlobalThread *threads[TEST_COUNT];
	AvgProfiler prof(TEST_COUNT);
	for(size_t i = 0; i < TEST_COUNT; ++i) {
		prof.start();
		threads[i] = GlobalThread::create(dummy,CPU::current().log_id(),String(""));
		prof.stop();
	}

	WVPERF(prof.avg(),"cycles for thread creation");
	WVPRINTF("min: %Lu",prof.min());
	WVPRINTF("max: %Lu",prof.max());

	for(size_t i = 0; i < TEST_COUNT; ++i)
		delete threads[i];
}
