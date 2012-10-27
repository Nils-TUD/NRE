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

namespace nre {

template<class T>
class DList;
template<class T>
class DListIterator;

/**
 * A listitem for the doubly linked list. It is intended that you inherit from this class to add
 * data to the item.
 */
class DListItem {
	template<class T>
	friend class DList;
	template<class T>
	friend class DListIterator;

public:
	/**
	 * Constructor
	 */
	explicit DListItem() : _prev(), _next() {
	}

private:
	DListItem *prev() {
		return _prev;
	}
	void prev(DListItem *i) {
		_prev = i;
	}
	DListItem *next() {
		return _next;
	}
	void next(DListItem *i) {
		_next = i;
	}

	DListItem *_prev;
	DListItem *_next;
};

/**
 * Generic iterator for a doubly linked list. Expects the list node class to have a prev() and
 * next() method.
 */
template<class T>
class DListIterator {
public:
	explicit DListIterator(T *p = 0, T *n = 0) : _p(p), _n(n) {
	}

	T & operator*() const {
		return *_n;
	}
	T *operator->() const {
		return &operator*();
	}
	DListIterator<T>& operator--() {
		_n = _p;
		if(_p)
			_p = static_cast<T*>(_p->prev());
		return *this;
	}
	DListIterator<T> operator--(int) {
		DListIterator<T> tmp(*this);
		operator--();
		return tmp;
	}
	DListIterator<T>& operator++() {
		_p = _n;
		_n = static_cast<T*>(_n->next());
		return *this;
	}
	DListIterator<T> operator++(int) {
		DListIterator<T> tmp(*this);
		operator++();
		return tmp;
	}
	bool operator==(const DListIterator<T>& rhs) const {
		return _n == rhs._n;
	}
	bool operator!=(const DListIterator<T>& rhs) const {
		return _n != rhs._n;
	}

private:
	T *_p;
	T *_n;
};

/**
 * The doubly linked list. Takes an arbitrary class as list-item and expects it to have a prev(),
 * prev(T*), next() and next(*T) method. In most cases, you should inherit from DListItem and
 * specify your class for T.
 */
template<class T>
class DList {
public:
	typedef DListIterator<T> iterator;

	/**
	 * Constructor. Creates an empty list
	 */
	explicit DList() : _head(0), _tail(0), _len(0) {
	}

	/**
	 * @return the number of items in the list
	 */
	size_t length() const {
		return _len;
	}

	/**
	 * @return beginning of list
	 */
	iterator begin() const {
		return iterator(0, _head);
	}
	/**
	 * @return end of list
	 */
	iterator end() const {
		return iterator(_tail, 0);
	}

	/**
	 * Appends the given item to the list. This works in constant time.
	 *
	 * @param e the list item
	 * @return the position where it has been inserted
	 */
	iterator append(T *e) {
		if(_head == 0)
			_head = e;
		else
			_tail->next(e);
		e->prev(_tail);
		e->next(0);
		_tail = e;
		_len++;
		return iterator(static_cast<T*>(e->prev()), e);
	}
	/**
	 * Removes the given item from the list. This works in constant time.
	 * Expects that the item is in the list!
	 *
	 * @param e the list item
	 */
	void remove(T *e) {
		if(e->prev())
			e->prev()->next(e->next());
		if(e->next())
			e->next()->prev(e->prev());
		if(e == _head)
			_head = static_cast<T*>(e->next());
		if(e == _tail)
			_tail = static_cast<T*>(e->prev());
		_len--;
	}

private:
	T *_head;
	T *_tail;
	size_t _len;
};

}
