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
#include "tests/TreapTest.h"
#include "tests/SortedSListTest.h"
#include "tests/PingpongXPd.h"
#include "tests/MemOps.h"
#include "tests/ThreadsTest.h"
#include "tests/OStreamTest.h"

using namespace nre;
using namespace nre::test;

const TestCase testcases[] = {
    memcpytest,
    memsettest,
    threads,
    pingpong,
    pingpongxpd,
    catchex,
    delegateperf,
    utcbnest,
    utcbperf,
    dstest,
    slisttest,
    sortedslisttest,
    dlisttest,
    cyclertest1,
    cyclertest2,
    cyclertest3,
    regmng,
    maskfield,
    sharedmem,
    treaptest_inorder,
    treaptest_revorder,
    treaptest_randorder,
    treaptest_perf,
    ostream_writef,
    ostream_strops,
};

int main() {
    for(size_t i = 0; i < ARRAY_SIZE(testcases); ++i) {
        Serial::get() << "Testing " << testcases[i].name << "...\n";
        try {
            testcases[i].func();
        }
        catch(const Exception& e) {
            Serial::get() << e;
        }
        Serial::get() << "Done\n";
    }
    // until we have michals test-system...
    Serial::get() << "\n============================================\n";
    Serial::get() << "Total failures: " << WvTest::failures;
    Serial::get() << "\n============================================\n";
    return 0;
}
