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
 * RAII class for locks. Assumes that the used class template has the method down() to acquire
 * the lock and up() to release it.
 */
template<class T>
class ScopedLock {
public:
	/**
	 * Constructor. Acquires the lock.
	 *
	 * @param lock the pointer to the lock-object
	 */
	explicit ScopedLock(T *lock)
		: _lock(lock) {
		_lock->down();
	}

	/**
	 * Destructor. Releases the lock
	 */
	~ScopedLock() {
		_lock->up();
	}

private:
	ScopedLock(const ScopedLock&);
	ScopedLock& operator=(const ScopedLock&);

	T *_lock;
};

}
