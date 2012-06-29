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
#include <arch/ExecEnv.h>
#include <stream/OStream.h>

namespace nul {

class Util {
public:
	template<typename T>
	static void swap(T &t1,T &t2) {
		T tmp = t1;
		t1 = t2;
		t2 = tmp;
	}

	static void write_backtrace(OStream& os) {
		uintptr_t addrs[32];
		ExecEnv::collect_backtrace(addrs,sizeof(addrs));
		write_backtrace(os,addrs);
	}
	static void write_backtrace(OStream& os,const uintptr_t *addrs) {
		const uintptr_t *addr;
		os.writef("Backtrace:\n");
		addr = addrs;
		while(*addr != 0) {
			os.writef("\t%p\n",*addr);
			addr++;
		}
	}

	// TODO move that to somewhere else
	static inline void pause() {
		asm volatile("pause");
	}
	static inline uint64_t tsc() {
		uint32_t u,l;
		asm volatile("rdtsc" : "=a"(l), "=d"(u));
		return (uint64_t)u << 32 | l;
	}

private:
	Util();
};

}
