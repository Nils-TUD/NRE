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

#include <arch/ExecEnv.h>
#include <arch/Startup.h>
#include <kobj/Ec.h>
#include <kobj/LocalEc.h>
#include <RCU.h>

/**
 * The return address for GlobalEc-functions, which will terminate the Ec.
 */
EXTERN_C void ec_landing_spot(void);

namespace nul {

class Utcb;

/**
 * A GlobalEc is an execution context to which you can bind a scheduling context. That means, it
 * is a "freely running" thread, in contrast to a LocalEc which does only serve portal-calls. Note
 * that you always have to bind a Sc to a GlobalEc to finally start it.
 */
class GlobalEc : public Ec {
	friend void ::_post_init();

	/**
	 * Creates the startup GlobalEc
	 *
	 * @param utcb the startup utcb address
	 * @param cap the capability selector for the GlobalEc
	 * @param cpu the CPU
	 * @param pd the protection domain
	 * @param stack the stack address
	 */
	explicit GlobalEc(uintptr_t utcb,capsel_t cap,cpu_t cpu,Pd *pd,uintptr_t stack)
			: Ec(cpu,0,cap,stack,utcb) {
		ExecEnv::set_current_ec(this);
		ExecEnv::set_current_pd(pd);
	}

public:
	typedef ExecEnv::startup_func startup_func;

	/**
	 * Creates a new GlobalEc that starts at <start> on CPU <cpu>.
	 *
	 * @param start the entry-point of the Ec
	 * @param cpu the CPU to bind the Ec to
	 * @param pd the protection-domain (default: Pd::current())
	 * @param utcb the utcb-address (0 = select it automatically)
	 */
	explicit GlobalEc(startup_func start,cpu_t cpu,Pd *pd = Pd::current(),uintptr_t utcb = 0)
			: Ec(cpu,Hip::get().service_caps() * cpu,INVALID,0,utcb) {
		create(pd,Syscalls::EC_GLOBAL,ExecEnv::setup_stack(pd,this,start,
				reinterpret_cast<uintptr_t>(ec_landing_spot),stack()));
	}
	/**
	 * Creates a new GlobalEc that starts at <start> on CPU <cpu> with a custom event-base.
	 *
	 * @param start the entry-point of the Ec
	 * @param cpu the CPU to bind the Ec to
	 * @param event_base the event-base to use for this Ec
	 * @param pd the protection-domain (default: Pd::current())
	 * @param utcb the utcb-address (0 = select it automatically)
	 */
	explicit GlobalEc(startup_func start,cpu_t cpu,capsel_t event_base,Pd *pd = Pd::current(),
			uintptr_t utcb = 0) : Ec(cpu,event_base,INVALID,0,utcb) {
		create(pd,Syscalls::EC_GLOBAL,ExecEnv::setup_stack(pd,this,start,
				reinterpret_cast<uintptr_t>(ec_landing_spot),stack()));
	}

private:
	static GlobalEc _cur;
};

}
