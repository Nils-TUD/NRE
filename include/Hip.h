/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <arch/Startup.h>
#include <util/Util.h>

namespace nul {

class HipCPU;
class HipMem;

/**
 * Hypervisor information page
 */
class Hip {
public:
	typedef const HipMem* mem_iterator;
	typedef const HipCPU* cpu_iterator;

	enum {
		MAX_CPUS	= 64,	// has to be a power of 2
		MAX_GSIS	= 128
	};

	/**
	 * @return the Hip
	 */
	static const Hip &get() {
		return *_startup_info.hip;
	}

private:
	uint32_t signature;
	uint16_t checksum;		// HIP checksum
	uint16_t length;		// HIP length
	uint16_t cpu_offs;		// Offset of first CPU descriptor
	uint16_t cpu_size;
	uint16_t mem_offs;		// Offset of first MEM descriptor
	uint16_t mem_size;
	uint32_t api_flg;		// API feature flags
public:
	uint32_t api_ver;		// API version
	uint32_t cfg_cap;		// Number of CAPs (SEL)
	uint32_t cfg_exc;		// Number of Exception portals (EXC)
	uint32_t cfg_vm;		// Number of VM portals (VMI)
	uint32_t cfg_gsi;		// Number of GSIs
	uint32_t cfg_page;		// PAGE sizes
	uint32_t cfg_utcb;		// UTCB sizes
	uint32_t freq_tsc;		// TSC freq in khz
	uint32_t freq_bus;		// BUS freq in khz

	/**
	 * @return the number of capabilities used by the HV for exceptions and for service-portals
	 */
	capsel_t service_caps() const {
		return cfg_exc * 2;
	}
	/**
	 * @return the first capability used for object capabilities
	 */
	capsel_t object_caps() const {
		return service_caps() * MAX_CPUS;
	}

	/**
	 * @return true if Intel VMX is supported
	 */
	bool has_vmx() const {
		return api_flg & (1 << 1);
	}
	/**
	 * @return true if AMD SVM is supported
	 */
	bool has_svm() const {
		return api_flg & (1 << 2);
	}

	/**
	 * @return the number of memory items
	 */
	size_t mem_count() const {
		return (length - mem_offs) / mem_size;
	}

	/**
	 * @return the number of CPU items (NOT online CPUs)
	 */
	size_t cpu_count() const {
		return (mem_offs - cpu_offs) / cpu_size;
	}

	/**
	 * Determines the logical CPU id for the given physical CPU id
	 *
	 * @param cpu the physical CPU id
	 * @return the logical CPU id
	 */
	cpu_t cpu_phys_to_log(cpu_t cpu) const;

	/**
	 * @return beginning of memory items
	 */
	mem_iterator mem_begin() const {
		return reinterpret_cast<mem_iterator>(reinterpret_cast<const char*>(this) + mem_offs);
	}
	/**
	 * @return end of memory items
	 */
	mem_iterator mem_end() const {
		return reinterpret_cast<mem_iterator>(reinterpret_cast<const char*>(this) + length);
	}

	/**
	 * @return beginning of CPU items
	 */
	cpu_iterator cpu_begin() const {
		return reinterpret_cast<cpu_iterator>(reinterpret_cast<const char*>(this) + cpu_offs);
	}
	/**
	 * @return end of CPU items
	 */
	cpu_iterator cpu_end() const {
		return reinterpret_cast<cpu_iterator>(reinterpret_cast<const char*>(this) + mem_offs);
	}
};

/**
 * CPU item in the Hip
 */
class HipCPU {
public:
	uint8_t flags;
	uint8_t thread;
	uint8_t core;
	uint8_t package;
private:
	uint32_t reserved;

public:
	/**
	 * @return the id of this CPU (0 .. n-1 in the order they appear in the Hip)
	 */
	cpu_t id() const {
		return this - Hip::get().cpu_begin();
	}
	/**
	 * @return true if this CPU is online
	 */
	bool enabled() const {
		return (flags & 1) != 0;
	}
};

/**
 * Memory item in the Hip
 */
class HipMem {
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

	bool reserved() const {
		return type != HipMem::AVAILABLE && type != HipMem::HYPERVISOR && type != HipMem::MB_MODULE;
	}
};

}
