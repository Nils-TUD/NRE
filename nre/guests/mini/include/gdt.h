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

class GDT {
private:
	enum {
		NOSYS           = 1 << 4,
		PRESENT         = 1 << 7,
		DPL             = 1 << 5 | 1 << 6
	};

	enum {
		TYPE_DATA       = 0 | NOSYS,
		TYPE_CODE       = 1 << 3 | NOSYS,
	};

	enum {
		DATA_WRITE      = 1 << 1,
		CODE_READ       = 1 << 1,
	};

	enum {
		PMODE_32BIT     = 1 << 6,
		PAGE_GRANU      = 1 << 7
	};

	enum {
		DPL_KERNEL      = 0,
		DPL_USER        = 3
	};

	struct Table {
		uint16_t size;      /* the size of the table -1 (size=0 is not allowed) */
		uint32_t offset;
	} A_PACKED;

	struct Desc {
		uint16_t sizeLow;
		uint16_t addrLow;
		uint8_t addrMiddle;
		uint8_t access;
		uint8_t sizeHigh;
		uint8_t addrHigh;
	} A_PACKED;

public:
	static void init();

private:
	static void set(Desc *gdt, size_t index, uintptr_t address, size_t size, uint8_t access,
	                uint8_t ringLevel);
	static void flush(Table* tbl);

	GDT();
	~GDT();
	GDT(const GDT&);
	GDT& operator=(const GDT&);

private:
	static Desc gdt[];
};
