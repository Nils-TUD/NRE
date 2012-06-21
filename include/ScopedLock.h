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
class ScopedLock {
public:
	explicit ScopedLock(T *lock)
		: _lock(lock) {
		_lock->down();
	}
	~ScopedLock() {
		_lock->up();
	}

private:
	ScopedLock(const ScopedLock&);
	ScopedLock& operator=(const ScopedLock&);

	T *_lock;
};

}
