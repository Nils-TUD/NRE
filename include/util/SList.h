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

#pragma once

#include <arch/Types.h>
#include <Assert.h>

namespace nre {

class SListItem {
public:
	SListItem() : _next() {
	}

	SListItem *next() {
		return _next;
	}
	void next(SListItem *i) {
		_next = i;
	}

private:
	SListItem *_next;
};

/**
 * Generic iterator for a singly linked list. Expects the list node class to have a next() method.
 */
template<class T>
class SListIterator {
public:
	explicit SListIterator(T *n = 0) : _n(n) {
	}

	T& operator *() const {
		return *_n;
	}
	T *operator ->() const {
		return &operator *();
	}
	SListIterator<T>& operator ++() {
		_n = static_cast<T*>(_n->next());
		return *this;
	}
	SListIterator<T> operator ++(int) {
		SListIterator<T> tmp(*this);
		operator++();
		return tmp;
	}
	bool operator ==(const SListIterator<T>& rhs) const {
		return _n == rhs._n;
	}
	bool operator !=(const SListIterator<T>& rhs) const {
		return _n != rhs._n;
	}

private:
	T *_n;
};

template<class T>
class SList {
public:
	typedef SListIterator<T> iterator;

	SList() : _head(0), _tail(0), _len(0) {
	}

	size_t length() const {
		return _len;
	}

	iterator begin() const {
		return iterator(_head);
	}
	iterator end() const {
		return iterator();
	}

	iterator append(T *e) {
		if(_head == 0)
			_head = e;
		else
			_tail->next(e);
		_tail = e;
		e->next(0);
		_len++;
		return iterator(e);
	}
	void remove(T *e) {
		T *t = _head, *p = 0;
		while(t != e) {
			p = t;
			t = static_cast<T*>(t->next());
		}
		assert(t == e);
		if(p)
			p->next(e->next());
		else
			_head = static_cast<T*>(e->next());
		if(!e->next())
			_tail = p;
		_len--;
	}

private:
	T *_head;
	T *_tail;
	size_t _len;
};

}
