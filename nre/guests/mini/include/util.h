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

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;

typedef unsigned long int size_t;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef unsigned long uintptr_t;
typedef unsigned long size_t;

#define assert(x)
#define ARRAY_SIZE(a)	(sizeof((a)) / sizeof((a)[0]))
#define A_PACKED		__attribute__((packed))
#define A_ALIGNED(x)	__attribute__((aligned (x)))

class Util {
public:
	static inline void set_cr0(uint32_t val) {
		asm (
			"mov	%0,%%cr0"
			: : "r" (val)
		);
	}

	static inline uint32_t get_cr0() {
		uint32_t res;
		asm (
			"mov	%%cr0,%0"
				: "=r" (res)
		);
		return res;
	}

	static inline void set_cr3(uint32_t val) {
		asm (
			"mov	%0,%%cr3"
			: : "r" (val)
		);
	}

	static inline void enable_ints() {
		asm volatile ("sti");
	}

	static inline void disable_ints() {
		asm volatile ("cli");
	}

	static void copy(void* dst,const void* src,size_t len);
	static void set(void* addr,int value,size_t count);
	static void move(void* dest,const void* src,size_t count);

private:
	Util();
	~Util();
	Util(const Util&);
	Util& operator=(const Util&);
};
