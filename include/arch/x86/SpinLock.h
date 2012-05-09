/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <Types.h>

static inline void lock(uint *val) {
	asm volatile (
		"mov	$1,%%ecx;"
		"1:"
		"	xor		%%eax,%%eax;"
		"	lock	cmpxchg %%ecx,(%0);"
		"	jz		2f;"
		/* improves the performance and lowers the power-consumption of spinlocks */
		"	pause;"
		"	jmp		1b;"
		"2:;"
		/* outputs */
		:
		/* inputs */
		: "D" (val)
		/* eax, ecx and cc will be clobbered; we need memory as well because *l is changed */
		: "eax", "ecx", "cc", "memory"
	);
}
static inline void unlock(uint *val) {
	*val = 0;
}

#ifdef __cplusplus
namespace nul {

class SpinLock {
public:
	SpinLock() : _val() {
	}

	void down() {
		lock(&_val);
	}
	void up() {
		unlock(&_val);
	}

private:
	SpinLock(const SpinLock&);
	SpinLock& operator=(const SpinLock&);

    uint _val;
};

}
#endif
