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

#include <util/SortedSList.h>

#include "SortedSListTest.h"

using namespace nre;
using namespace nre::test;

static void test_sortedslist();

const TestCase sortedslisttest = {
	"Sorted singly linked list",test_sortedslist
};

struct ListItem : public SListItem {
	ListItem(int v) : SListItem(), val(v) {
	}
	int val;
};

static bool isless(const ListItem &a,const ListItem &b) {
	return a.val < b.val;
}

static void test_sortedslist() {
	ListItem e1(10),e2(1),e3(-5),e4(8),e5(12);
	SortedSList<ListItem>::iterator it;
	SortedSList<ListItem> l(isless);
	WVPASSEQ(l.length(),static_cast<size_t>(0));
	WVPASS(l.begin() == l.end());

	l.insert(&e1);
	l.insert(&e2);
	l.insert(&e3);
	l.insert(&e4);
	l.insert(&e5);
	WVPASSEQ(l.length(),static_cast<size_t>(5));
	it = l.begin();
	WVPASS(&*it == &e3);
	++it;
	WVPASS(&*it == &e2);
	++it;
	WVPASS(&*it == &e4);
	++it;
	WVPASS(&*it == &e1);
	++it;
	WVPASS(&*it == &e5);
	++it;
	WVPASS(it == l.end());

	l.remove(&e2);
	l.remove(&e1);
	l.remove(&e5);
	WVPASSEQ(l.length(),static_cast<size_t>(2));
	it = l.begin();
	WVPASS(&*it == &e3);
	++it;
	WVPASS(&*it == &e4);
	++it;
	WVPASS(it == l.end());
}
