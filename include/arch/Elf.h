/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#ifdef __i386__
#include <arch/x86_32/Elf.h>
#elif defined __x86_64__
#include <arch/x86_64/Elf.h>
#else
#error "Unsupported architecture"
#endif

#include <arch/Types.h>

namespace nul {

struct ElfEh {
	uchar e_ident[16];
	elf_half_t e_type;
	elf_half_t e_machine;
	elf_word_t e_version;
	elf_addr_t e_entry;
	elf_off_t e_phoff;
	elf_off_t e_shoff;
	elf_word_t e_flags;
	elf_half_t e_ehsz;
	elf_half_t e_phentsize;
	elf_half_t e_phnum;
	elf_half_t e_shentsize;
	elf_half_t e_shnum;
	elf_half_t e_shstrndx;
};

enum ElfPhFlags {
	PF_X	= 1 << 0,
	PF_W	= 1 << 1,
	PF_R	= 1 << 2,
};

// TODO thats not nice
struct ElfPh {
	elf_word_t p_type;
#ifdef __x86_64__
	elf_word_t p_flags;
#endif
	elf_off_t p_offset;
	elf_addr_t p_vaddr;
	elf_addr_t p_paddr;
#ifdef __x86_64__
	elf_addr_t p_filesz;
	elf_addr_t p_memsz;
	elf_addr_t p_align;
#else
	elf_word_t p_filesz;
	elf_word_t p_memsz;
	elf_word_t p_flags;
	elf_word_t p_align;
#endif
};

struct ElfSh {
	elf_word_t sh_name;
	elf_word_t sh_type;
	elf_word_t sh_flags;
	elf_addr_t sh_addr;
	elf_off_t sh_offset;
	elf_word_t sh_size;
	elf_word_t sh_link;
	elf_word_t sh_info;
	elf_word_t sh_addralign;
	elf_word_t sh_entsize;
};

}
