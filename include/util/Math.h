/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <arch/Types.h>

namespace nul {

class Math {
public:
	template<typename T>
	static inline T min(T a,T b) {
		return a < b ? a : b;
	}

	template<typename T>
	static inline T max(T a,T b) {
		return a < b ? b : a;
	}

	template<typename T>
	static inline T round_up(T value,T align) {
		return (value + align - 1) & ~(align - 1);
	}

	template<typename T>
	static inline T round_dn(T value,T align) {
		return value & ~(align - 1);
	}

	/**
	 * @return the number of blocks of size <blocksize> that <value> contains
	 */
	template<typename T>
	static inline T blockcount(T value,T blocksize) {
		return (value + blocksize - 1) / blocksize;
	}

	/**
	 * @return whether the ranges [<b1>..<b1>+<s1>) and [<b2>..<b2>+<s2>) overlap somewhere.
	 */
	static inline bool overlapped(uint64_t b1,size_t s1,uint64_t b2,size_t s2) {
		// note that we use 64-bit address here to support the Hip-memory-entries
		uint64_t e1 = b1 + s1;
		uint64_t e2 = b2 + s2;
		return (b1 >= b2 && b1 < e2) ||			// start in range
				(e1 > b2 && e1 <= e2) ||		// end in range
				(b1 < b2 && e1 > e2);			// completely overlapped
	}

	/**
	 * @return the position of the most significant "1" bit
	 */
	template<typename T>
	static inline T bit_scan_reverse(T value) {
		return __builtin_clz(value) ^ 0x1F;
	}

	/**
	 * @return the position of the least significant "1" bit
	 */
	template<typename T>
	static inline T bit_scan_forward(T value) {
		return __builtin_ctz(value);
	}

	template<typename T>
	static inline T is_pow2(T value) {
		return (value & (value - 1)) == 0;
	}

	/**
	 * @return the prev power of two. That is, if it is already a power of 2 it returns the input. If
	 * 	not it rounds down to the prev power of two.
	 */
	template<typename T>
	static inline T prev_pow2(T value) {
		return 1 << prev_pow2_shift(value);
	}
	template<typename T>
	static inline T prev_pow2_shift(T value) {
		if(value <= 1)
			return value;
		if(!is_pow2(value))
			return bit_scan_reverse(value) - 1;
		return bit_scan_reverse(value);
	}

	/**
	 * @return the next power of two. That is, if it is already a power of 2 it returns the input. If
	 * 	not it rounds up to the next power of two.
	 */
	template<typename T>
	static inline T next_pow2(T value) {
		return 1 << next_pow2_shift(value);
	}
	template<typename T>
	static inline T next_pow2_shift(T value) {
		if(!value)
			return 0;
		if(!is_pow2(value))
			return bit_scan_reverse(value) + 1;
		return bit_scan_reverse(value);
	}

	/**
	 * Calculates the order (log2 of the size) of the largest naturally
	 * aligned block that starts at start and is no larger than size.
	 *
	 * @param start the start of the block
	 * @param size the size of the block
	 * @return The calculated order or the minshift parameter is it is
	 * smaller then the order.
	 */
	static inline uint minshift(uintptr_t start,size_t size) {
		uint basealign = static_cast<uint>(bit_scan_forward(
				start | (1ul << (8 * sizeof(uintptr_t) - 1))));
		uint shiftalign = static_cast<uint>(bit_scan_reverse(size | 1));
		return min(basealign,shiftalign);
	}

private:
	Math();
};

}
