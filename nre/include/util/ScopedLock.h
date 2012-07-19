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

namespace nre {

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
