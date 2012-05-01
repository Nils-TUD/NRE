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

#include <mem/RegionList.h>
#include "RegionListTest.h"

using namespace nul;
using namespace nul::test;

static void test_reglist();

const TestCase regionlist = {
	"Regionlist",test_reglist
};

static void test_reglist() {
	uintptr_t src;

	{
		RegionList l;
		l.add(0x1000,0x4000,0x1000,RegionList::RW);
		l.add(0x8000,0x2000,0x10000,RegionList::R);
		l.add(0x10000,0x8000,0x20000,RegionList::RX);
		WVPASSEQ(l.regcount(),3UL);
		WVPASSEQ(l.find(0x1000,src),static_cast<uint>(RegionList::RW));
		WVPASSEQ(src,0x1000UL);
		WVPASSEQ(l.find(0x2123,src),static_cast<uint>(RegionList::RW));
		WVPASSEQ(src,0x2000UL);
		WVPASSEQ(l.find(0x9100,src),static_cast<uint>(RegionList::R));
		WVPASSEQ(src,0x11000UL);
		WVPASSEQ(l.find(0x10100,src),static_cast<uint>(RegionList::RX));
		WVPASSEQ(src,0x20000UL);
	}

	{
		RegionList l;
		l.add(0x1000,0x4000,0x1000,RegionList::RW);
		l.add(0x8000,0x2000,0x10000,RegionList::R);
		l.add(0x10000,0x8000,0x20000,RegionList::RX);
		WVPASSEQ(l.regcount(),3UL);
		l.remove(0x2000,0x1000);
		WVPASSEQ(l.regcount(),4UL);
		WVPASSEQ(l.find(0x2000,src),0U);
		WVPASSEQ(l.find(0x1000,src),static_cast<uint>(RegionList::RW));
		WVPASSEQ(src,0x1000UL);
		WVPASSEQ(l.find(0x3000,src),static_cast<uint>(RegionList::RW));
		WVPASSEQ(src,0x3000UL);
	}

	{
		RegionList l;
		l.add(0x1000,0x4000,0x1000,RegionList::RW);
		l.add(0x8000,0x2000,0x10000,RegionList::R);
		l.add(0xA000,0x8000,0x20000,RegionList::RX);
		l.remove(0x2000,0x1000);
		l.remove(0x1000,0x1000);
		l.remove(0x8000,0x4000);
		l.remove(0xE000,0x6000);
		l.remove(0xC000,0x2000);
		l.remove(0x3000,0x2000);
		WVPASSEQ(l.regcount(),0UL);
	}

	{
		RegionList l;
		l.add(0x1000,0x4000,0x1000,RegionList::RW);
		l.add(0x8000,0x2000,0x10000,RegionList::R);
		l.add(0xA000,0x8000,0x20000,RegionList::RX);

		l.map(0x1000);
		WVPASSEQ(l.find(0x1000,src),static_cast<uint>(RegionList::RW | RegionList::M));
		WVPASSEQ(src,0x1000UL);
		WVPASSEQ(l.find(0x2123,src),static_cast<uint>(RegionList::RW));
		WVPASSEQ(src,0x2000UL);
		WVPASSEQ(l.find(0x3000,src),static_cast<uint>(RegionList::RW));
		WVPASSEQ(src,0x3000UL);
		WVPASSEQ(l.find(0x4000,src),static_cast<uint>(RegionList::RW));
		WVPASSEQ(src,0x4000UL);

		l.map(0x9000);
		WVPASSEQ(l.find(0x9000,src),static_cast<uint>(RegionList::R | RegionList::M));
		WVPASSEQ(src,0x11000UL);
		WVPASSEQ(l.find(0x8000,src),static_cast<uint>(RegionList::R));
		WVPASSEQ(src,0x10000UL);

		l.unmap(0x9000);
		WVPASSEQ(l.find(0x9000,src),static_cast<uint>(RegionList::R));
		WVPASSEQ(src,0x11000UL);
		WVPASSEQ(l.find(0x8000,src),static_cast<uint>(RegionList::R));
		WVPASSEQ(src,0x10000UL);

		l.map(0x4000);
		WVPASSEQ(l.find(0x4000,src),static_cast<uint>(RegionList::RW | RegionList::M));
		WVPASSEQ(src,0x4000UL);

		uintptr_t addr = l.find_free(0x1000);
		WVPASSEQ(l.find(addr,src),0U);
		l.add(addr,0x1000,0,RegionList::RWX);
		WVPASSEQ(l.find(addr,src),static_cast<uint>(RegionList::RWX));
		addr = l.find_free(0x1000);
		WVPASSEQ(l.find(addr,src),0U);

		l.remove(0x1000,0x20000);
		WVPASSEQ(l.regcount(),0UL);
	}
}
