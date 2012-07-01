/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <arch/Types.h>
#include <Assert.h>

namespace nul {

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
			t = t->next();
		}
		assert(t == e);
		if(p)
			p->next(e->next());
		else
			_head = e->next();
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
