/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <Types.h>

class Hip_cpu {
public:
	uint8_t flags;
	uint8_t thread;
	uint8_t core;
	uint8_t package;
private:
	uint32_t reserved;

public:
	bool enabled() const {
		return (flags & 1) != 0;
	}
};

class Hip_mem {
public:
	enum {
		AVAILABLE	= 1,
		HYPERVISOR	= -1,
		MB_MODULE	= -2
	};

	uint64_t addr;
	uint64_t size;
	int32_t type;
	uint32_t aux;
};

class Hip {
public:
	typedef const Hip_mem* mem_const_iterator;
	typedef const Hip_cpu* cpu_const_iterator;

	uint32_t signature;
	uint16_t checksum;		// HIP checksum
	uint16_t length;		// HIP length
	uint16_t cpu_offs;		// Offset of first CPU descriptor
	uint16_t cpu_size;
	uint16_t mem_offs;		// Offset of first MEM descriptor
	uint16_t mem_size;
	uint32_t api_flg;		// API feature flags
	uint32_t api_ver;		// API version
	uint32_t cfg_cap;		// Number of CAPs (SEL)
	uint32_t cfg_exc;		// Number of Exception portals (EXC)
	uint32_t cfg_vm;		// Number of VM portals (VMI)
	uint32_t cfg_gsi;		// Number of GSIs
	uint32_t cfg_page;		// PAGE sizes
	uint32_t cfg_utcb;		// UTCB sizes
	uint32_t freq_tsc;		// TSC freq in khz
	uint32_t freq_bus;		// BUS freq in khz

	bool has_vmx() const {
		return api_flg & (1 << 1);
	}
	bool has_svm() const {
		return api_flg & (1 << 2);
	}

	size_t mem_count() const {
		return (length - mem_offs) / mem_size;
	}

	size_t cpu_count() const {
		return (mem_offs - cpu_offs) / cpu_size;
	}

	mem_const_iterator mem_begin() const {
		return reinterpret_cast<mem_const_iterator>(reinterpret_cast<const char*>(this) + mem_offs);
	}
	mem_const_iterator mem_end() const {
		return reinterpret_cast<mem_const_iterator>(reinterpret_cast<const char*>(this) + length);
	}

	cpu_const_iterator cpu_begin() const {
		return reinterpret_cast<cpu_const_iterator>(reinterpret_cast<const char*>(this) + cpu_offs);
	}
	cpu_const_iterator cpu_end() const {
		return reinterpret_cast<cpu_const_iterator>(reinterpret_cast<const char*>(this) + mem_offs);
	}
};