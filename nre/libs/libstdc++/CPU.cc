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

#include <arch/Startup.h>
#include <kobj/Pt.h>
#include <cap/CapSelSpace.h>
#include <CPU.h>
#include <Hip.h>

EXTERN_C void dlmalloc_init();

namespace nre {

class CPUInit {
	CPUInit();
	static CPUInit init;
};

size_t CPU::_count = 0;
uint CPU::_order = 0;
CPU *CPU::_online = 0;
CPU CPU::_cpus[Hip::MAX_CPUS] INIT_PRIO_CPUS;
cpu_t CPU::_logtophys[Hip::MAX_CPUS];
CPUInit CPUInit::init INIT_PRIO_CPUS;

CPUInit::CPUInit() {
	CPU *last = 0;
	const Hip& hip = Hip::get();
	cpu_t i = 0, id = 0, offline = 0;
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it, ++i) {
		CPU &cpu = CPU::get(i);
		if(it->enabled())
			cpu._id = id++;
		else
			cpu._id = Hip::MAX_CPUS - ++offline;
		CPU::_logtophys[cpu._id] = it->id();
		if(!it->enabled())
			continue;

		// build a list of all online CPUs
		CPU::_count++;
		if(last)
			last->_next = &cpu;
		else
			CPU::_online = &cpu;
		cpu._next = 0;
		last = &cpu;

		// copy attributes from Hip-CPU
		cpu._flags = it->flags;
		cpu._package = it->package;
		cpu._core = it->core;
		cpu._thread = it->thread;

		// create per-cpu-portals
		if(_startup_info.child) {
			capsel_t off = cpu.log_id() * Hip::get().service_caps();
			cpu._ds_pt = new Pt(off + CapSelSpace::SRV_DS);
			cpu._srv_pt = new Pt(off + CapSelSpace::SRV_SERVICE);
			cpu._gsi_pt = new Pt(off + CapSelSpace::SRV_GSI);
			cpu._io_pt = new Pt(off + CapSelSpace::SRV_IO);
			cpu._sc_pt = new Pt(off + CapSelSpace::SRV_SC);
			if(cpu.phys_id() == _startup_info.cpu) {
				// switch to dlmalloc, since we have created its dependencies now
				// note: by doing it here, the startup-heap-size does not depend on the number of CPUs
				dlmalloc_init();
			}
		}
	}
	CPU::_order = Math::next_pow2_shift(CPU::_count);
}

}
