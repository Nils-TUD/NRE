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

#include <util/CPUSet.h>
#include <Hip.h>
#include <Assert.h>

namespace nre {

/**
 * This class allows to configure the Hip for a child Pd.
 */
class ChildHip : public Hip {
public:
	/**
	 * Creates a basic Hip for a Child based on your Hip
	 *
	 * @param cpuset the CPUs to mark available
	 */
	explicit ChildHip(const CPUSet &cpuset = CPUSet(CPUSet::ALL)) : cpus(), reserved() {
		// the cast is necessary to access the protected members of Hip
		copy(static_cast<const ChildHip&>(Hip::get()), cpuset);
	}

	/**
	 * Adds a module with given parameters to the memory list
	 *
	 * @param addr the physical address
	 * @param size the size (in bytes)
	 * @param cmdline the command line
	 */
	void add_module(uint64_t addr, uint64_t size, const char *cmdline) {
		add_mem(addr, size, reinterpret_cast<word_t>(cmdline), HipMem::MB_MODULE);
	}
	/**
	 * Adds a memory item to the list.
	 *
	 * @param addr the physical address
	 * @param size the size (in bytes)
	 * @param aux the aux field (the commandline for modules)
	 * @param type the type of item
	 */
	void add_mem(uint64_t addr, uint64_t size, word_t aux, int32_t type) {
		assert(size > 0);
		size_t i = memcount();
		assert(reinterpret_cast<uintptr_t>(&mems[i] + 1) <=
		       reinterpret_cast<uintptr_t>(this) + ExecEnv::PAGE_SIZE);
		mems[i].addr = addr;
		mems[i].size = size;
		mems[i].type = type;
		mems[i].aux = aux;
	}

	/**
	 * Finalizes the Hip, i.e. calculates the length and the checksum
	 */
	void finalize() {
		length = mem_offs + memcount() * mem_size;
		checksum = calc_checksum();
	}

private:
	size_t memcount() const {
		size_t memcount;
		for(memcount = 0; mems[memcount].size != 0; ++memcount)
			;
		return memcount;
	}

	void copy(const ChildHip& hip, const CPUSet &cpuset) {
		signature = hip.signature;
		checksum = 0;
		api_flg = hip.api_flg;
		api_ver = hip.api_ver;
		cfg_cap = hip.cfg_cap;
		cfg_exc = hip.cfg_exc;
		cfg_vm = hip.cfg_vm;
		cfg_gsi = hip.cfg_gsi;
		cfg_page = hip.cfg_page;
		cfg_utcb = hip.cfg_utcb;
		freq_tsc = hip.freq_tsc;
		freq_bus = hip.freq_bus;
		cpu_size = hip.cpu_size;
		mem_size = hip.mem_size;
		// note that this assumes that MAX_CPUS is the same as NUM_CPU in NOVA
		cpu_offs = hip.cpu_offs;
		mem_offs = hip.mem_offs;
		for(CPU::iterator cpu = CPU::begin(); cpu != CPU::end(); ++cpu) {
			if(cpuset.get().is_set(cpu->log_id())) {
				cpus[cpu->phys_id()].flags = cpu->flags();
				cpus[cpu->phys_id()].thread = cpu->thread();
				cpus[cpu->phys_id()].core = cpu->core();
				cpus[cpu->phys_id()].package = cpu->package();
			}
			else
				cpus[cpu->phys_id()].flags = 0;
		}
	}

	HipCPU cpus[MAX_CPUS];
	union {
		HipMem mems[];
		uint8_t reserved[(ExecEnv::PAGE_SIZE - sizeof(Hip) - sizeof(cpus))];
	};
};

}
