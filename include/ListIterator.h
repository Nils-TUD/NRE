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

namespace nul {

template<class T>
class ListIterator {
public:
	ListIterator(T *n = 0) : _n(n) {
	}
	~ListIterator() {
	}

	T& operator *() const {
		return *_n;
	}
	T *operator ->() const {
		return &operator *();
	}
	ListIterator<T>& operator ++() {
		_n = _n->next();
		return *this;
	}
	ListIterator<T> operator ++(int) {
		ListIterator<T> tmp(*this);
		operator++();
		return tmp;
	}
	bool operator ==(const ListIterator<T>& rhs) {
		return _n == rhs._n;
	}
	bool operator !=(const ListIterator<T>& rhs) {
		return _n != rhs._n;
	}

private:
	T *_n;
};

}
