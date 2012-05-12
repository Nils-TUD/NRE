/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <kobj/Sm.h>
#include <Util.h>

namespace nul {

/**
 * A user semaphore optimized for the case where we do not block.
 * This semaphore implementation breaks when threads blocking on the
 * associated kernel semaphore can be canceled, because the kernel
 * semaphore and the shared counter might then be out-of-sync.
 */
class UserSm {
	Sm _sem;
	long _value;
public:
	UserSm() : _sem(0), _value(1) {
	}

	void down() {
		if(Util::atomic_xadd(&_value,-1) <= 0)
			_sem.down();
	}
	void up() {
		if(Util::atomic_xadd(&_value,+1) < 0)
			_sem.up();
	}
};

}
