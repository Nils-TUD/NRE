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
	static T xadd(T volatile *ptr,Y value) {
		return __sync_fetch_and_add(ptr,value);
	}

	/**
	 * Compare and swap
	 */
	template<typename T,typename Y>
	static bool swap(T volatile *ptr,Y oldval,Y newval) {
		return __sync_bool_compare_and_swap(ptr,oldval,newval);
	}
};

}
