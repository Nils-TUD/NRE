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
#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <cap/CapSelSpace.h>

namespace nre {

GlobalThread GlobalThread::_cur INIT_PRIO_GEC(_startup_info.utcb, CapSelSpace::INIT_EC,
                                              CapSelSpace::INIT_SC, _startup_info.cpu,
                                              &Pd::_cur, _startup_info.stack);

GlobalThread::GlobalThread(uintptr_t uaddr, capsel_t gt, capsel_t sc, cpu_t cpu, Pd *pd, uintptr_t stack)
	: Thread(Hip::get().cpu_phys_to_log(cpu), 0, gt, stack, uaddr), _sc(new Sc(this, sc, pd)),
	  _name("main") {
	ExecEnv::set_current_thread(this);
	ExecEnv::set_current_pd(pd);
}

GlobalThread::~GlobalThread() {
	delete _sc;
}

void GlobalThread::start(Qpd qpd, Pd *pd) {
	assert(_sc == 0);
	_sc = new Sc(this, qpd, pd);
	_sc->start(_name);
}

}
