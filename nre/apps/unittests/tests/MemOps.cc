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
#include <cstdlib>

#include "MemOps.h"

using namespace nre;
using namespace nre::test;

typedef void (*memop_func)(void *a,void *b,size_t len);

static void test_memcpy();
static void test_memset();
static void do_test(const char *name,memop_func func);

const TestCase memcpytest = {
	"Memory operations",test_memcpy
};
const TestCase memsettest = {
	"Memory operations",test_memset
};

static const size_t AREA_SIZE	= 4096;
static const uint TEST_COUNT	= 1000;

static void memcpy_func(void *a,void *b,size_t len) {
	memcpy(a,const_cast<const void*>(b),len);
}
static void memset_func(void *a,void *,size_t len) {
	memset(a,0,len);
}

static void test_memcpy() {
	do_test("memcpy",memcpy_func);
}
static void test_memset() {
	do_test("memset",memset_func);
}

static void do_test(const char *name,memop_func func) {
	void *mem = malloc(AREA_SIZE);
	void *buf = malloc(AREA_SIZE);

	{
		WVPRINTF("Testing aligned %s with %zu bytes",name,AREA_SIZE);
		AvgProfiler prof(TEST_COUNT);
		for(uint i = 0; i < TEST_COUNT; ++i) {
			prof.start();
			func(buf,mem,AREA_SIZE);
			prof.stop();
		}
		WVPERF(prof.avg()," cycles");
		WVPRINTF("min: %Lu",prof.min());
		WVPRINTF("max: %Lu",prof.max());
	}

	{
		WVPRINTF("Testing unaligned %s with %zu bytes",name,AREA_SIZE);
		AvgProfiler prof(TEST_COUNT);
		for(uint i = 0; i < TEST_COUNT; ++i) {
			prof.start();
			func(reinterpret_cast<char*>(buf) + 1,reinterpret_cast<char*>(mem) + 1,AREA_SIZE - 2);
			prof.stop();
		}
		WVPERF(prof.avg()," cycles");
		WVPRINTF("min: %Lu",prof.min());
		WVPRINTF("max: %Lu",prof.max());
	}

	free(buf);
	free(mem);
}
