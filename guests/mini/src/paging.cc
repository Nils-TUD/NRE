#include "paging.h"

static Paging::pde_t pd[Paging::PAGE_SIZE / sizeof(Paging::pde_t)] A_ALIGNED(Paging::PAGE_SIZE);
static Paging::pte_t pt[Paging::PAGE_SIZE / sizeof(Paging::pte_t)] A_ALIGNED(Paging::PAGE_SIZE);

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
	assert(virt < PAGE_SIZE * PAGE_SIZE);
	assert((virt & (PAGE_SIZE - 1)) == 0);
	assert((phys & (PAGE_SIZE - 1)) == 0);
	pt[virt / PAGE_SIZE] = phys | flags;
	flush(virt);
}

