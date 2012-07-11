/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <mem/RegionManager.h>
#include <util/ScopedPtr.h>

#include "RegMngTest.h"

using namespace nre;
using namespace nre::test;

static void test_regmng();

const TestCase regmng = {
	"RegionManager",test_regmng
};

void test_regmng() {
	uintptr_t addr1,addr2,addr3;
	{
		ScopedPtr<RegionManager> rm(new RegionManager());
		rm->free(0x100000,0x3000);
		rm->free(0x200000,0x1000);
		rm->free(0x280000,0x2000);

		addr1 = rm->alloc(0x1000);
		addr2 = rm->alloc(0x2000);
		addr3 = rm->alloc(0x2000);

		rm->free(addr1,0x1000);
		rm->free(addr2,0x2000);
		rm->free(addr3,0x2000);

		RegionManager::iterator it = rm->begin();
		WVPASSEQ(it->addr,0x100000UL);
		WVPASSEQ(it->size,0x3000UL);
		++it;
		WVPASSEQ(it->addr,0x200000UL);
		WVPASSEQ(it->size,0x1000UL);
		++it;
		WVPASSEQ(it->addr,0x280000UL);
		WVPASSEQ(it->size,0x2000UL);
	}

	{
		ScopedPtr<RegionManager> rm(new RegionManager());
		rm->free(0x100000,0x3000);
		rm->free(0x200000,0x1000);
		rm->free(0x280000,0x2000);

		addr1 = rm->alloc(0x1000);
		addr2 = rm->alloc(0x1000);
		addr3 = rm->alloc(0x1000);

		rm->free(addr1,0x1000);
		rm->free(addr3,0x1000);
		rm->free(addr2,0x1000);

		RegionManager::iterator it = rm->begin();
		WVPASSEQ(it->addr,0x100000UL);
		WVPASSEQ(it->size,0x3000UL);
		++it;
		WVPASSEQ(it->addr,0x200000UL);
		WVPASSEQ(it->size,0x1000UL);
		++it;
		WVPASSEQ(it->addr,0x280000UL);
		WVPASSEQ(it->size,0x2000UL);
	}
}
