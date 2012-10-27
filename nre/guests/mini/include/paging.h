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

#include "util.h"

#define PAGE_SHIFT      12
#define PAGE_SIZE       (1 << PAGE_SHIFT)

class Paging {
private:
	enum {
		CR0_WRITEPROT   = 1 << 16,
		CR0_PAGING      = 1 << 31
	};

public:
	typedef uint32_t pde_t;
	typedef uint32_t pte_t;

	enum {
		PRESENT         = 1 << 0,
		WRITABLE        = 1 << 1
	};

	static void init();

	static void map(uintptr_t addr, uintptr_t phys, int flags);
	static void set_writeprot(bool prot) {
		if(prot)
			Util::set_cr0(Util::get_cr0() | CR0_WRITEPROT);
		else
			Util::set_cr0(Util::get_cr0() & ~CR0_WRITEPROT);
	}

private:
	static inline void flush(uintptr_t addr) {
		__asm__(
		    "invlpg (%0)"
			: : "r" (addr)
		);
	}

	Paging();
	~Paging();
	Paging(const Paging&);
	Paging& operator=(const Paging&);
};

