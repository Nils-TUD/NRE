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

/**
 * RAII class for dynamic memory.
 */
template<class T>
class ScopedPtr {
public:
	/**
	 * Constructor
	 *
	 * @param obj pointer to the object that has been allocated with new
	 */
	explicit ScopedPtr(T *obj)
		: _obj(obj) {
	}
	/**
	 * Destructor. Uses delete to free the object, if it hasn't been released previously.
	 */
	~ScopedPtr() {
		delete _obj;
	}

	/**
	 * @return the object
	 */
	T *get() {
		return _obj;
	}
	/**
	 * @return the object
	 */
	T *operator->() {
		return _obj;
	}

	/**
	 * Specifies that the object should be kept, i.e. should not be deleted during construction
	 * of this ScopedPtr object
	 *
	 * @return the object
	 */
	T *release() {
		T *res = _obj;
		_obj = 0;
		return res;
	}

private:
	ScopedPtr(const ScopedPtr&);
	ScopedPtr& operator=(const ScopedPtr&);

	T *_obj;
};

}
