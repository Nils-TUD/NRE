/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include <util/DList.h>

#include "DListTest.h"

using namespace nul;
using namespace nul::test;

static void test_dlist();

const TestCase dlisttest = {
	"Doubly linked list",test_dlist
};

static void test_dlist() {
	DListItem e1,e2,e3;
	DList<DListItem>::iterator it;
	DList<DListItem> l;
	WVPASSEQ(l.length(),0UL);
	WVPASS(l.begin() == l.end());

	l.append(&e1);
	l.append(&e2);
	l.append(&e3);
	WVPASSEQ(l.length(),3UL);
	it = l.begin();
	WVPASS(&*it == &e1);
	++it;
	WVPASS(&*it == &e2);
	++it;
	WVPASS(&*it == &e3);
	++it;
	WVPASS(it == l.end());
	--it;
	WVPASS(&*it == &e3);
	--it;
	WVPASS(&*it == &e2);
	--it;
	WVPASS(&*it == &e1);
	WVPASS(it == l.begin());

	l.remove(&e2);
	WVPASSEQ(l.length(),2UL);
	it = l.begin();
	WVPASS(&*it == &e1);
	++it;
	WVPASS(&*it == &e3);
	++it;
	WVPASS(it == l.end());
	--it;
	WVPASS(&*it == &e3);
	--it;
	WVPASS(&*it == &e1);
	WVPASS(it == l.begin());

	l.remove(&e3);
	WVPASSEQ(l.length(),1UL);
	it = l.begin();
	WVPASS(&*it == &e1);
	++it;
	WVPASS(it == l.end());
	--it;
	WVPASS(&*it == &e1);
	WVPASS(it == l.begin());

	l.append(&e3);
	WVPASSEQ(l.length(),2UL);
	it = l.begin();
	WVPASS(&*it == &e1);
	++it;
	WVPASS(&*it == &e3);
	++it;
	WVPASS(it == l.end());
	--it;
	WVPASS(&*it == &e3);
	--it;
	WVPASS(&*it == &e1);
	WVPASS(it == l.begin());

	l.remove(&e1);
	l.remove(&e3);
	WVPASSEQ(l.length(),0UL);
	WVPASS(l.begin() == l.end());

	l.append(&e2);
	WVPASSEQ(l.length(),1UL);
	it = l.begin();
	WVPASS(&*it == &e2);
	++it;
	WVPASS(it == l.end());
	--it;
	WVPASS(&*it == &e2);
	WVPASS(it == l.begin());
}
