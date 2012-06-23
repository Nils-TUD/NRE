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

#include <arch/Startup.h>
#include <kobj/GlobalEc.h>
#include <kobj/Pd.h>
#include <kobj/Pt.h>
#include <kobj/Sm.h>
#include <utcb/UtcbFrame.h>
#include <Exception.h>
#include <CPU.h>
#include <pthread.h>

using namespace nul;

EXTERN_C void abort();
EXTERN_C void dlmalloc_init();

// is overwritten by the root-task; all others don't need it
WEAK void *_stack;

// TODO the setup-stuff is not really nice. I think we need init-priorities or something

static void verbose_terminate() {
	try {
		throw;
	}
	catch(const Exception& e) {
		Serial::get() << e;
	}
	catch(...) {
		Serial::get() << "Uncatched, unknown exception\n";
	}
	abort();
}

void _presetup() {
	static Pd initpd(CapSpace::INIT_PD);
	static GlobalEc initec(_startup_info.utcb,CapSpace::INIT_EC,_startup_info.cpu,
			&initpd,_startup_info.stack);
	// force the linker to include the pthread object-file. FIXME why is this necessary??
	pthread_cancel(0);
}

void _setup(bool child) {
	if(child) {
		// grab our initial caps (pd, ec, sc) from parent
		UtcbFrame uf;
		uf.set_receive_crd(Crd(CapSpace::INIT_PD,2,Crd::OBJ_ALL));
		Pt initpt(CapSpace::SRV_INIT);
		initpt.call(uf);
	}

	// we can't create the semaphore before we've set Pd::current() and until the pd-cap is valid
	// (see above; childs have to fetch it from their parent)
	RCU::init();
	// now we can use the semaphore to announce this Ec
	RCU::announce(Ec::current());

	CPU *last = 0;
	const Hip& hip = Hip::get();
	for(Hip::cpu_iterator it = hip.cpu_begin(); it != hip.cpu_end(); ++it) {
		CPU &cpu = CPU::get(it->id());
		cpu.id = it->id();
		if(it->enabled()) {
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
			if(child) {
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
					// note: by doing it here, the startup-heap-size does not depend on the number
					// of CPUs
					dlmalloc_init();
				}
			}
		}
	}
	_startup_info.done = true;

	if(child) {
		try {
			Serial::get().init();
		}
		catch(...) {
			// ignore it when it fails. this happens for the log-service, which of course can't
			// connect to itself ;)
		}
	}
	std::set_terminate(verbose_terminate);
}
