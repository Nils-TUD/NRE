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

#include "paging.h"

static Paging::pde_t pd[PAGE_SIZE / sizeof(Paging::pde_t)] A_ALIGNED(PAGE_SIZE);
static Paging::pte_t pt[PAGE_SIZE / sizeof(Paging::pte_t)] A_ALIGNED(PAGE_SIZE);

void Paging::init() {
	uintptr_t addr = 0;
	for(size_t i = 0; i < 1024; i++) {
		pt[i] = addr | PRESENT | WRITABLE;
		addr += PAGE_SIZE;
	}
	pd[0] = (uintptr_t)pt | PRESENT | WRITABLE;

	// now set page-dir and enable paging
	Util::set_cr3((uint32_t)pd);
	Util::set_cr0(Util::get_cr0() | CR0_PAGING);
}

void Paging::map(uintptr_t virt,uintptr_t phys,int flags) {
	assert(virt < PAGE_SIZE * (PAGE_SIZE / sizeof(pte)));
	assert((virt & (PAGE_SIZE - 1)) == 0);
	assert((phys & (PAGE_SIZE - 1)) == 0);
	pt[virt / PAGE_SIZE] = phys | flags;
	flush(virt);
}

