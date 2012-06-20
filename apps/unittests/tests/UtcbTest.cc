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

const TestCase utcbtest = {
	"Utcb nesting",test_nesting
};

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
	LocalEc ec(CPU::current().id);
	Pt pt(&ec,portal_test);

	UtcbFrame uf;
	uf << 4 << 1 << 2;
	uf.delegate(CapRange(12,16,Crd::IO_ALL,10));
	uf.delegate(CapRange(1,2,Crd::MEM_ALL,2),UtcbFrame::NONE);

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
