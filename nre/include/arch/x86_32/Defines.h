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

#define REG(X)				e##X
#define ARCH_REGIONS_END	0x10000000
#define ARCH_KERNEL_START	0xC0000000
#define ARCH_PAGE_SIZE		0x1000
#define FMT_WORD_HEXLEN		"8"
#define FMT_WORD_BYTES		"4"
#define ASM_WORD_TYPE		".long"
