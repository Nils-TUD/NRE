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

namespace nre {

typedef ushort elf32_half_t;
typedef uint elf32_word_t;
typedef uint elf32_addr_t;
typedef uint elf32_off_t;

struct ElfEh32 {
	uchar e_ident[16];
	elf32_half_t e_type;
	elf32_half_t e_machine;
	elf32_word_t e_version;
	elf32_addr_t e_entry;
	elf32_off_t e_phoff;
	elf32_off_t e_shoff;
	elf32_word_t e_flags;
	elf32_half_t e_ehsz;
	elf32_half_t e_phentsize;
	elf32_half_t e_phnum;
	elf32_half_t e_shentsize;
	elf32_half_t e_shnum;
	elf32_half_t e_shstrndx;
};

struct ElfPh32 {
	elf32_word_t p_type;
	elf32_off_t p_offset;
	elf32_addr_t p_vaddr;
	elf32_addr_t p_paddr;
	elf32_word_t p_filesz;
	elf32_word_t p_memsz;
	elf32_word_t p_flags;
	elf32_word_t p_align;
};

struct ElfSh32 {
	elf32_word_t sh_name;
	elf32_word_t sh_type;
	elf32_word_t sh_flags;
	elf32_addr_t sh_addr;
	elf32_off_t sh_offset;
	elf32_word_t sh_size;
	elf32_word_t sh_link;
	elf32_word_t sh_info;
	elf32_word_t sh_addralign;
	elf32_word_t sh_entsize;
};

}
