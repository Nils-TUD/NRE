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

#include <util/SList.h>

namespace nre {

template<class T>
class SortedSList {
public:
	typedef typename SList<T>::iterator iterator;
	typedef bool (*cmp_func)(const T &a,const T &b);

	explicit SortedSList(cmp_func isless) : _isless(isless), _list() {
	}

	size_t length() const {
		return _list.length();
	}

	iterator begin() const {
		return _list.begin();
	}
	iterator end() const {
		return _list.end();
	}
	iterator tail() const {
		return _list.tail();
	}

	iterator insert(T *e) {
		T *p = 0;
		iterator it;
		for(it = begin(); it != end() && _isless(*it,*e); ++it)
			p = &*it;
		return _list.insert(p,e);
	}
	void remove(T *e) {
		_list.remove(e);
	}

private:
	cmp_func _isless;
	SList<T> _list;
};

}
