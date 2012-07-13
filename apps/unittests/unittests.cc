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

#include <stream/Serial.h>

#include "tests/Pingpong.h"
#include "tests/UtcbTest.h"
#include "tests/DelegatePerf.h"
#include "tests/CatchEx.h"
#include "tests/SharedMemory.h"
#include "tests/DataSpaceTest.h"
#include "tests/SListTest.h"
#include "tests/DListTest.h"
#include "tests/CyclerTest.h"
#include "tests/RegMngTest.h"
#include "tests/MaskFieldTest.h"

using namespace nre;
using namespace nre::test;

const TestCase testcases[] = {
	pingpong,
	catchex,
	delegateperf,
	utcbnest,
	utcbperf,
	dstest,
	slisttest,
	dlisttest,
	cyclertest1,
	cyclertest2,
	cyclertest3,
	regmng,
	maskfield,
	sharedmem,
};

int main() {
	for(size_t i = 0; i < ARRAY_SIZE(testcases); ++i) {
		Serial::get().writef("Testing %s...\n",testcases[i].name);
		try {
			testcases[i].func();
		}
		catch(const Exception& e) {
			Serial::get() << e;
		}
		Serial::get().writef("Done\n");
	}
}
