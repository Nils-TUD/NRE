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

#include <kobj/Pt.h>
#include <cap/CapSpace.h>
#include <CPU.h>
#include <Hip.h>

EXTERN_C void dlmalloc_init();

namespace nul {

class CPUInit {
	CPUInit();
	static CPUInit init;
};

CPU *CPU::_online = 0;
CPU CPU::_cpus[Hip::MAX_CPUS] INIT_PRIO_CPUS;
CPUInit CPUInit::init INIT_PRIO_CPUS;

CPUInit::CPUInit() {
	CPU *last = 0;
	const Hip& hip = Hip::get();
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		CPU &cpu = CPU::get(it->id());
		cpu.id = it->id();
		if(!it->enabled())
			continue;

		// build a list of all online CPUs
		if(last)
			last->_next = &cpu;
		else
			CPU::_online = &cpu;
		cpu._next = 0;
		last = &cpu;

		// copy attributes from Hip-CPU
		cpu.flags = it->flags;
		cpu.package = it->package;
		cpu.core = it->core;
		cpu.thread = it->thread;

		// create per-cpu-portals
		if(_startup_info.child) {
			capsel_t off = cpu.id * Hip::get().service_caps();
			cpu.map_pt = new Pt(off + CapSpace::SRV_MAP);
			cpu.unmap_pt = new Pt(off + CapSpace::SRV_UNMAP);
			cpu.reg_pt = new Pt(off + CapSpace::SRV_REG);
			cpu.unreg_pt = new Pt(off + CapSpace::SRV_UNREG);
			cpu.get_pt = new Pt(off + CapSpace::SRV_GET);
			cpu.gsi_pt = new Pt(off + CapSpace::SRV_GSI);
			cpu.io_pt = new Pt(off + CapSpace::SRV_IO);
			if(cpu.id == CPU::current().id) {
				// switch to dlmalloc, since we have created its dependencies now
				// note: by doing it here, the startup-heap-size does not depend on the number of CPUs
				dlmalloc_init();
			}
		}
	}
}

}
