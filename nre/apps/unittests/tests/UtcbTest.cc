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

#include "UtcbTest.h"

using namespace nre;
using namespace nre::test;

PORTAL static void portal_test(capsel_t);
static void test_nesting();
static void test_perf();

const TestCase utcbnest = {
    "Utcb nesting", test_nesting
};
const TestCase utcbperf = {
    "Utcb performance", test_perf
};
static const uint tries = 100000;

static void portal_test(capsel_t) {
    UtcbFrameRef uf;
    try {
        int a, b, c;
        uf >> a >> b >> c;
        if(a == 0xDEAD) {
            uf.get_delegated(0);
            uf.get_delegated(0);
            uf.accept_delegates();
        }
        uf.clear();
        uf << c << b << a;
    }
    catch(const Exception &e) {
        Serial::get() << e;
        WVPASS(false);
        uf.clear();
    }
}

static void test_nesting() {
    int a, b, c;
    LocalThread *ec = LocalThread::create(CPU::current().log_id());
    {
        UtcbFrameRef uf(ec->utcb());
        uf.accept_delegates(1);
    }
    Pt pt(ec, portal_test);

    UtcbFrame uf;
    uf << 0xDEAD << 1 << 2;
    uf.delegate(CapSelSpace::INIT_EC, 0);
    uf.delegate(CapSelSpace::INIT_PD, 1);

    {
        UtcbFrame uf1;
        uf1 << 1 << 2 << 3;
        {
            UtcbFrame uf2;
            uf2 << 4 << 5 << 6;
            pt.call(uf2);

            uf2 >> a >> b >> c;
            WVPASSEQ(a, 6);
            WVPASSEQ(b, 5);
            WVPASSEQ(c, 4);
        }
        WVPASSEQ(uf1.typed(), static_cast<size_t>(0));
        WVPASSEQ(uf1.untyped(), static_cast<size_t>(3));

        pt.call(uf1);

        uf1 >> a >> b >> c;
        WVPASSEQ(a, 3);
        WVPASSEQ(b, 2);
        WVPASSEQ(c, 1);
    }
    WVPASSEQ(uf.typed(), static_cast<size_t>(2));
    WVPASSEQ(uf.untyped(), static_cast<size_t>(3));

    pt.call(uf);

    uf >> a >> b >> c;
    WVPASSEQ(a, 2);
    WVPASSEQ(b, 1);
    WVPASSEQ(c, 0xDEAD);
}

static void perform_test_unnested(AvgProfiler &prof) {
    for(uint i = 0; i < tries; i++) {
        prof.start();
        {
            UtcbFrame uf;
            uf << 1;
        }
        prof.stop();
    }
}

static void perform_test(size_t n, AvgProfiler &prof) {
    for(uint i = 0; i < tries; i++) {
        {
            UtcbFrame uf;
            for(size_t x = 0; x < n; ++x)
                uf << x;

            prof.start();
            {
                UtcbFrame nested;
                nested << 1;
            }
            prof.stop();
        }
    }
}

static void test_perf() {
    {
        AvgProfiler prof(tries);
        perform_test_unnested(prof);
        WVPRINT("Without nesting:");
        WVPERF(prof.avg(), "cycles");
        WVPRINT("min: " << prof.min());
        WVPRINT("max: " << prof.max());
    }

    size_t sizes[] = {1, 2, 4, 8, 16, 32, 64, 128};
    for(size_t i = 0; i < ARRAY_SIZE(sizes); ++i) {
        AvgProfiler prof(tries);
        perform_test(sizes[i], prof);
        WVPRINT("Using " << sizes[i] << " words:");
        WVPERF(prof.avg(), "cycles");
        WVPRINT("min: " << prof.min());
        WVPRINT("max: " << prof.max());
    }
}
