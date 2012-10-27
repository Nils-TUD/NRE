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

#include "gdt.h"
#include "paging.h"

GDT::Desc GDT::gdt[3];

void GDT::init() {
	Table table;
	table.offset = (uintptr_t)gdt;
	table.size = sizeof(gdt) - 1;

	set(gdt, 1, 0, 0xFFFFFFFF >> PAGE_SHIFT, TYPE_CODE | PRESENT | CODE_READ, DPL_KERNEL);
	set(gdt, 2, 0, 0xFFFFFFFF >> PAGE_SHIFT, TYPE_DATA | PRESENT | DATA_WRITE, DPL_KERNEL);
	flush(&table);
}

void GDT::set(Desc *gdt, size_t index, uintptr_t address, size_t size, uint8_t access,
              uint8_t ringLevel) {
	gdt[index].addrLow = address & 0xFFFF;
	gdt[index].addrMiddle = (address >> 16) & 0xFF;
	gdt[index].addrHigh = (address >> 24) & 0xFF;
	gdt[index].sizeLow = size & 0xFFFF;
	gdt[index].sizeHigh = ((size >> 16) & 0xF) | PAGE_GRANU | PMODE_32BIT;
	gdt[index].access = access | ((ringLevel & 3) << 5);
}

void GDT::flush(Table* tbl) {
	asm volatile (
	    "lgdt	(%0)\n"             // load gdt
	    "mov	$0x10,%%eax\n"      // set the value for the segment-registers
	    "mov	%%eax,%%ds\n"       // reload segments
	    "mov	%%eax,%%es\n"
	    "mov	%%eax,%%fs\n"
	    "mov	%%eax,%%gs\n"
	    "mov	%%eax,%%ss\n"
	    "ljmp	$0x08,$1f\n"        // reload code-segment via far-jump
	    "1:\n"
		: : "r" (tbl) : "eax"
	);
}
