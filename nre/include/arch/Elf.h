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

#include <arch/Types.h>

#ifdef __i386__
#    include <arch/x86_32/Elf.h>
#elif defined __x86_64__
#    include <arch/x86_64/Elf.h>
#else
#    error "Unsupported architecture"
#endif

namespace nre {

enum ElfPhFlags {
	PF_X    = 1 << 0,
	PF_W    = 1 << 1,
	PF_R    = 1 << 2,
};

#ifdef __i386__
typedef ElfEh32 ElfEh;
typedef ElfPh32 ElfPh;
typedef ElfSh32 ElfSh;
#elif defined __x86_64__
typedef ElfEh64 ElfEh;
typedef ElfPh64 ElfPh;
typedef ElfSh64 ElfSh;
#endif

}
