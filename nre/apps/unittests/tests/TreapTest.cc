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

#include <util/Util.h>
#include <util/Treap.h>
#include <util/Random.h>

#include "TreapTest.h"

#define TEST_NODE_COUNT		10
#define PERF_NODE_COUNT		5000

using namespace nre;
using namespace nre::test;

static void test_in_order();
static void test_rev_order();
static void test_rand_order();
static void test_perf();
static void test_add_and_rem(int *vals);
static void store_perf(size_t i,uint64_t rdtsc,uint64_t &min,uint64_t &max,uint64_t time);
static void print_perf(const char *name,uint64_t min,uint64_t max);

const TestCase treaptest_inorder = {
	"Treap - add and remove nodes with increasing values",test_in_order,
};
const TestCase treaptest_revorder = {
	"Treap - add and remove nodes with decreasing values",test_rev_order,
};
const TestCase treaptest_randorder = {
	"Treap - add and remove regions with addresses in rand order",test_rand_order
};
const TestCase treaptest_perf = {
	"Treap - performance",test_perf
};

static uint64_t results[PERF_NODE_COUNT];

struct MyNode : public TreapNode<int> {
	MyNode(int key,int _data) : TreapNode<int>(key), data(_data) {
	}
	int data;
};

static void test_in_order() {
	static int vals[TEST_NODE_COUNT];
	for(size_t i = 0; i < TEST_NODE_COUNT; i++)
		vals[i] = i;
	test_add_and_rem(vals);
}

static void test_rev_order() {
	static int vals[TEST_NODE_COUNT];
	for(size_t i = 0; i < TEST_NODE_COUNT; i++)
		vals[i] = TEST_NODE_COUNT - i;
	test_add_and_rem(vals);
}

static void test_rand_order() {
	static int vals[TEST_NODE_COUNT];
	for(size_t i = 0; i < TEST_NODE_COUNT; i++)
		vals[i] = i;
	Random::init(0x12345);
	for(size_t i = 0; i < 10000; i++) {
		size_t j = Random::get() % TEST_NODE_COUNT;
		size_t k = Random::get() % TEST_NODE_COUNT;
		uintptr_t t = vals[j];
		vals[j] = vals[k];
		vals[k] = t;
	}
	test_add_and_rem(vals);
}

static void test_perf() {
	Treap<MyNode> tree;
	MyNode **nodes = new MyNode*[PERF_NODE_COUNT];

	uint64_t tic,tac,min,max,rdtsc;
	tic = Util::tsc();
	tac = Util::tsc();
	rdtsc = tac - tic;

	/* create */
	min = ~0ull;
	max = 0;
	for(size_t i = 0; i < PERF_NODE_COUNT; i++) {
		nodes[i] = new MyNode(i,i);

		tic = Util::tsc();
		tree.insert(nodes[i]);
		tac = Util::tsc();
		store_perf(i,rdtsc,min,max,tac - tic);
	}

	print_perf("Node insertion:",min,max);

	/* find all */
	min = ~0ull;
	max = 0;
	for(size_t i = 0; i < PERF_NODE_COUNT; i++) {
		tic = Util::tsc();
		tree.find(i);
		tac = Util::tsc();
		store_perf(i,rdtsc,min,max,tac - tic);
	}

	print_perf("Node searching:",min,max);

	/* remove */
	min = ~0ull;
	max = 0;
	for(size_t i = 0; i < PERF_NODE_COUNT; i++) {
		tic = Util::tsc();
		tree.remove(nodes[i]);
		tac = Util::tsc();
		store_perf(i,rdtsc,min,max,tac - tic);
	}

	print_perf("Node removal:",min,max);

	for(size_t i = 0; i < PERF_NODE_COUNT; i++)
		delete nodes[i];
	delete[] nodes;
}

static void test_add_and_rem(int *vals) {
	static MyNode *nodes[TEST_NODE_COUNT];
	Treap<MyNode> tree;
	MyNode *node;

	/* create */
	for(size_t i = 0; i < TEST_NODE_COUNT; i++) {
		nodes[i] = new MyNode(vals[i],i);
		tree.insert(nodes[i]);
	}

	/* find all */
	for(size_t i = 0; i < TEST_NODE_COUNT; i++) {
		node = tree.find(vals[i]);
		WVPASSEQPTR(node,nodes[i]);
	}

	/* remove */
	for(size_t i = 0; i < TEST_NODE_COUNT; i++) {
		tree.remove(nodes[i]);
		node = tree.find(vals[i]);
		WVPASSEQPTR(node,(MyNode*)0);
		delete nodes[i];

		for(size_t j = i + 1; j < TEST_NODE_COUNT; j++) {
			node = tree.find(vals[j]);
			WVPASSEQPTR(node,nodes[j]);
		}
	}
}

static void store_perf(size_t i,uint64_t rdtsc,uint64_t &min,uint64_t &max,uint64_t time) {
	uint64_t duration = time > rdtsc ? time - rdtsc : 0;
	min = Math::min(min,duration);
	max = Math::max(max,duration);
	results[i] = duration;
}

static void print_perf(const char *name,uint64_t min,uint64_t max) {
	uint64_t avg = 0;
	for(uint i = 0; i < PERF_NODE_COUNT; i++)
		avg += results[i];
	avg = avg / PERF_NODE_COUNT;
	WVPRINTF("%s",name);
	WVPERF(avg,"cycles");
	WVPRINTF("min: %Lu",min);
	WVPRINTF("max: %Lu",max);
}
