#pragma once

#include <stdint.h>
#include "util.h"

class Paging {
private:
	enum {
		CR0_WRITEPROT	= 1 << 16,
		CR0_PAGING		= 1 << 31
	};

public:
	typedef uint32_t pde_t;
	typedef uint32_t pte_t;

	enum {
		PAGE_SHIFT		= 12,
		PAGE_SIZE		= 1 << PAGE_SHIFT
	};

	enum {
		PRESENT			= 1 << 0,
		WRITABLE		= 1 << 1
	};

	static void init();

	static void map(uintptr_t addr,uintptr_t phys,int flags);
	static void set_writeprot(bool prot) {
		if(prot)
			Util::set_cr0(Util::get_cr0() | CR0_WRITEPROT);
		else
			Util::set_cr0(Util::get_cr0() & ~CR0_WRITEPROT);
	}

private:
	static inline void flush(uintptr_t addr) {
		__asm__ (
			"invlpg (%0)"
			: : "r" (addr)
		);
	}

	Paging();
	~Paging();
	Paging(const Paging&);
	Paging& operator=(const Paging&);
};

