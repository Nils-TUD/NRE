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

#include <util/MaskField.h>

#include "MaskFieldTest.h"

using namespace nre;
using namespace nre::test;

static void test_maskfield();

const TestCase maskfield = {
	"MaskField", test_maskfield
};

static void test_maskfield() {
	{
		MaskField<2> mf(8);
		mf.set_all();
		WVPASSEQ(mf.get(0), 0x3UL);
		WVPASSEQ(mf.get(1), 0x3UL);
		WVPASSEQ(mf.get(2), 0x3UL);
		WVPASSEQ(mf.get(3), 0x3UL);

		mf.clear_all();
		WVPASSEQ(mf.get(0), 0UL);
		WVPASSEQ(mf.get(1), 0UL);
		WVPASSEQ(mf.get(2), 0UL);
		WVPASSEQ(mf.get(3), 0UL);
	}

	{
		MaskField<4> mf(64);
		mf.set(0, 0xF);
		mf.set(1, 0x3);
		mf.set(2, 0x1);
		mf.set(3, 0x5);
		mf.set(4, 0x6);
		mf.set(10, 0x8);
		mf.set(11, 0x0);
		mf.set(12, 0xF);
		WVPASSEQ(mf.get(0), 0xFUL);
		WVPASSEQ(mf.get(1), 0x3UL);
		WVPASSEQ(mf.get(2), 0x1UL);
		WVPASSEQ(mf.get(3), 0x5UL);
		WVPASSEQ(mf.get(4), 0x6UL);
		WVPASSEQ(mf.get(10), 0x8UL);
		WVPASSEQ(mf.get(11), 0x0UL);
		WVPASSEQ(mf.get(12), 0xFUL);
	}
}
