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

#pragma once

#include <mem/Region.h>

namespace nul {

class RegionManager {
private:
	class Node {
	public:
		Region data;
		Node *next;
	};

	enum {
		MAX_REGIONS	= 128
	};

public:
	typedef const Node *iterator;

	RegionManager() : _begin(0), _end(0) {
		Node *n = _regs;
		n->next = 0;
		for(size_t i = 1; i < MAX_REGIONS; ++i) {
			_regs[i].next = n;
			_free = _regs + i;
		}
	}

	iterator begin() const {
		return _begin;
	}
	iterator end() const {
		return _end;
	}

	void add(const Region& r) {
		if(!_free)
			throw Exception("No free regions left");
		Node *n = _free;
		_free = _free->next;

		n->data = r;
		n->next = 0;
		if(!_begin)
			_begin = n;
		_end = n;
	}
	void remove(const Region& r) {

	}

private:
	Node *_begin;
	Node *_end;
	Node *_free;
	Node _regs[MAX_REGIONS];
};

}
