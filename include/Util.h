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

#include <Types.h>

class Util {
public:
	template<typename T>
	static inline T min(T a,T b) {
		return a < b ? a : b;
	}

	template<typename T>
	static inline T max(T a,T b) {
		return a < b ? b : a;
	}

	static uint64_t tsc() {
		uint32_t u,l;
		asm volatile("rdtsc" : "=a"(l), "=d"(u));
		return (uint64_t)u << 32 | l;
	}
};
