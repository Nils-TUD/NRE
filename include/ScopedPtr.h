/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

namespace nul {

template<class T>
class ScopedPtr {
public:
	explicit ScopedPtr(T *obj)
		: _obj(obj) {
	}
	~ScopedPtr() {
		delete _obj;
	}

	T *get() {
		return _obj;
	}
	T *release() {
		T *res = _obj;
		_obj = 0;
		return res;
	}
	T *operator->() {
		return _obj;
	}

private:
	ScopedPtr(const ScopedPtr&);
	ScopedPtr& operator=(const ScopedPtr&);

	T *_obj;
};

}
