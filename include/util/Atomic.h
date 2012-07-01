/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <arch/Types.h>

namespace nul {

class Atomic {
public:
	/**
	 * Adds <value> to *<ptr> and returns the old value
	 */
	template<typename T,typename Y>
	static T add(T volatile *ptr,Y value) {
		return __sync_fetch_and_add(ptr,value);
	}

	/**
	 * Compare and swap
	 */
	template<typename T,typename Y>
	static bool swap(T volatile *ptr,Y oldval,Y newval) {
		return __sync_bool_compare_and_swap(ptr,oldval,newval);
	}

	static uint64_t read_atonce(volatile uint64_t &v) {
		uint32_t lo = 0;
		uint32_t hi = 0;
		// We don't need a lock prefix, because this is only meant to be
		// uninterruptible not atomic. So don't use this for SMP!
		asm ("cmpxchg8b %2\n"
				: "+a" (static_cast<uint32_t>(lo)), "+d" (static_cast<uint32_t>(hi))
				: "m" (v), "b" (static_cast<uint32_t>(lo)), "c" (static_cast<uint32_t>(hi)));
		return (static_cast<uint64_t>(hi) << 32) | lo;
	}

	static void write_atonce(volatile uint64_t &to,uint64_t value) {
		to = value;
		uint32_t nlo = value;
		uint32_t nhi = value >> 32;
		uint32_t olo = 0;
		uint32_t ohi = 0;

		// We iterate at least twice to get the correct old value and then
		// to substitute it.
		asm ("1: cmpxchg8b %0\n"
				"jne 1b"
				: "+m" (to),
				"+a" (static_cast<uint32_t>(olo)), "+d" (static_cast<uint32_t>(ohi))
				: "b" (static_cast<uint32_t>(nlo)), "c" (static_cast<uint32_t>(nhi)));
	}

private:
	Atomic();
};

}
