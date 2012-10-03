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

typedef ushort elf64_half_t;
typedef uint elf64_word_t;
typedef ulong elf64_addr_t;
typedef ulong elf64_off_t;

struct ElfEh64 {
	uchar e_ident[16];
	elf64_half_t e_type;
	elf64_half_t e_machine;
	elf64_word_t e_version;
	elf64_addr_t e_entry;
	elf64_off_t e_phoff;
	elf64_off_t e_shoff;
	elf64_word_t e_flags;
	elf64_half_t e_ehsz;
	elf64_half_t e_phentsize;
	elf64_half_t e_phnum;
	elf64_half_t e_shentsize;
	elf64_half_t e_shnum;
	elf64_half_t e_shstrndx;
};

struct ElfPh64 {
	elf64_word_t p_type;
	elf64_word_t p_flags;
	elf64_off_t p_offset;
	elf64_addr_t p_vaddr;
	elf64_addr_t p_paddr;
	elf64_addr_t p_filesz;
	elf64_addr_t p_memsz;
	elf64_addr_t p_align;
};

struct ElfSh64 {
	elf64_word_t sh_name;
	elf64_word_t sh_type;
	elf64_word_t sh_flags;
	elf64_addr_t sh_addr;
	elf64_off_t sh_offset;
	elf64_word_t sh_size;
	elf64_word_t sh_link;
	elf64_word_t sh_info;
	elf64_word_t sh_addralign;
	elf64_word_t sh_entsize;
};

}
