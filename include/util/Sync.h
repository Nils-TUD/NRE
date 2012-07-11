/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

namespace nre {

class Sync {
public:
	/**
	 * Prevents the compiler from reordering instructions. That is, the code-generator will put all
	 * preceding load and store commands before load and store commands that follow this call.
	 */
	static inline void memory_barrier() {
		asm volatile ("" : : : "memory");
	}

	/**
	 * Ensures that all loads and stores before this call are globally visible before all loads and
	 * stores after this call.
	 */
	static inline void memory_fence() {
		asm volatile("mfence" : : : "memory");
	}
	/**
	 * Ensures that all loads before this call are globally visible before all loads after this call.
	 */
	static inline void load_fence() {
		asm volatile("lfence" : : : "memory");
	}
	/**
	 * Ensures that all stores before this call are globally visible before all stores after this call.
	 */
	static inline void store_fence() {
		asm volatile("sfence" : : : "memory");
	}

private:
	Sync();
};

}
