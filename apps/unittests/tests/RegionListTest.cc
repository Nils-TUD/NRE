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

#include <subsystem/ChildMemory.h>
#include <util/ScopedPtr.h>

#include "RegionListTest.h"

using namespace nul;
using namespace nul::test;

const TestCase regionlist = {
	"Regionlist",test_reglist
};

void test_reglist() {
	uintptr_t src = 0;
	size_t size;

	{
		// don't allocate it on the stack - would be too large
		ScopedPtr<ChildMemory> l(new ChildMemory());
		l->add(0x1000,0x4000,0x1000,ChildMemory::RW);
		l->add(0x8000,0x2000,0x10000,ChildMemory::R);
		l->add(0x10000,0x8000,0x20000,ChildMemory::RX);
		WVPASSEQ(l->regcount(),3UL);
		WVPASSEQ(l->find(0x1000,src,size),static_cast<uint>(ChildMemory::RW));
		WVPASSEQ(src,0x1000UL);
		WVPASSEQ(l->find(0x2123,src,size),static_cast<uint>(ChildMemory::RW));
		WVPASSEQ(src,0x2000UL);
		WVPASSEQ(l->find(0x9100,src,size),static_cast<uint>(ChildMemory::R));
		WVPASSEQ(src,0x11000UL);
		WVPASSEQ(l->find(0x10100,src,size),static_cast<uint>(ChildMemory::RX));
		WVPASSEQ(src,0x20000UL);
	}

	{
		ScopedPtr<ChildMemory> l(new ChildMemory());
		l->add(0x1000,0x4000,0x1000,ChildMemory::RW);
		l->add(0x8000,0x2000,0x10000,ChildMemory::R);
		l->add(0x10000,0x8000,0x20000,ChildMemory::RX);
		WVPASSEQ(l->regcount(),3UL);
		l->remove(0x2000,0x1000);
		WVPASSEQ(l->regcount(),4UL);
		WVPASSEQ(l->find(0x2000,src,size),0U);
		WVPASSEQ(l->find(0x1000,src,size),static_cast<uint>(ChildMemory::RW));
		WVPASSEQ(src,0x1000UL);
		WVPASSEQ(l->find(0x3000,src,size),static_cast<uint>(ChildMemory::RW));
		WVPASSEQ(src,0x3000UL);
	}

	{
		ScopedPtr<ChildMemory> l(new ChildMemory());
		l->add(0x1000,0x4000,0x1000,ChildMemory::RW);
		l->add(0x8000,0x2000,0x10000,ChildMemory::R);
		l->add(0xA000,0x8000,0x20000,ChildMemory::RX);
		l->remove(0x2000,0x1000);
		l->remove(0x1000,0x1000);
		l->remove(0x8000,0x4000);
		l->remove(0xE000,0x6000);
		l->remove(0xC000,0x2000);
		l->remove(0x3000,0x2000);
		WVPASSEQ(l->regcount(),0UL);
	}

	{
		ScopedPtr<ChildMemory> l(new ChildMemory());
		l->add(0x1000,0x4000,0x1000,ChildMemory::RW);
		l->add(0x8000,0x2000,0x10000,ChildMemory::R);
		l->add(0xA000,0x8000,0x20000,ChildMemory::RX);

		l->map(0x1000);
		WVPASSEQ(l->find(0x1000,src,size),static_cast<uint>(ChildMemory::RW | ChildMemory::M));
		WVPASSEQ(src,0x1000UL);
		WVPASSEQ(l->find(0x2123,src,size),static_cast<uint>(ChildMemory::RW));
		WVPASSEQ(src,0x2000UL);
		WVPASSEQ(l->find(0x3000,src,size),static_cast<uint>(ChildMemory::RW));
		WVPASSEQ(src,0x3000UL);
		WVPASSEQ(l->find(0x4000,src,size),static_cast<uint>(ChildMemory::RW));
		WVPASSEQ(src,0x4000UL);

		l->map(0x9000);
		WVPASSEQ(l->find(0x9000,src,size),static_cast<uint>(ChildMemory::R | ChildMemory::M));
		WVPASSEQ(src,0x11000UL);
		WVPASSEQ(l->find(0x8000,src,size),static_cast<uint>(ChildMemory::R));
		WVPASSEQ(src,0x10000UL);

		l->unmap(0x9000);
		WVPASSEQ(l->find(0x9000,src,size),static_cast<uint>(ChildMemory::R));
		WVPASSEQ(src,0x11000UL);
		WVPASSEQ(l->find(0x8000,src,size),static_cast<uint>(ChildMemory::R));
		WVPASSEQ(src,0x10000UL);

		l->map(0x4000);
		WVPASSEQ(l->find(0x4000,src,size),static_cast<uint>(ChildMemory::RW | ChildMemory::M));
		WVPASSEQ(src,0x4000UL);

		uintptr_t addr = l->find_free(0x1000);
		WVPASSEQ(l->find(addr,src,size),0U);
		l->add(addr,0x1000,0,ChildMemory::RWX);
		WVPASSEQ(l->find(addr,src,size),static_cast<uint>(ChildMemory::RWX));
		addr = l->find_free(0x1000);
		WVPASSEQ(l->find(addr,src,size),0U);

		l->remove(0x1000,0x20000);
		WVPASSEQ(l->regcount(),0UL);
	}
}
