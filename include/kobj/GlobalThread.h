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
#include <kobj/Thread.h>
#include <kobj/Pd.h>
#include <Hip.h>

/**
 * The return address for GlobalThread-functions, which will terminate the Thread.
 */
EXTERN_C void ec_landing_spot(void);

namespace nul {

class Utcb;

/**
 * A GlobalThread is a thread to which you can bind a scheduling context. That means, it
 * is a "freely running" thread, in contrast to a LocalThread which does only serve portal-calls. Note
 * that you always have to bind a Sc to a GlobalThread to finally start it.
 */
class GlobalThread : public Thread {
	friend void ::_post_init();

	/**
	 * Creates the startup GlobalThread
	 *
	 * @param uaddr the startup utcb address
	 * @param cap the capability selector for the GlobalThread
	 * @param cpu the physical CPU id
	 * @param pd the protection domain
	 * @param stack the stack address
	 */
	explicit GlobalThread(uintptr_t uaddr,capsel_t cap,cpu_t cpu,Pd *pd,uintptr_t stack)
			: Thread(Hip::get().cpu_phys_to_log(cpu),0,cap,stack,uaddr) {
		ExecEnv::set_current_thread(this);
		ExecEnv::set_current_pd(pd);
	}

public:
	typedef ExecEnv::startup_func startup_func;

	/**
	 * Creates a new GlobalThread that starts at <start> on CPU <cpu>.
	 *
	 * @param start the entry-point of the Thread
	 * @param cpu the logical CPU id to bind the Thread to
	 * @param pd the protection-domain (default: Pd::current())
	 * @param utcb the utcb-address (0 = select it automatically)
	 */
	explicit GlobalThread(startup_func start,cpu_t cpu,Pd *pd = Pd::current(),uintptr_t utcb = 0)
			: Thread(cpu,Hip::get().service_caps() * cpu,INVALID,0,utcb) {
		create(pd,Syscalls::EC_GLOBAL,ExecEnv::setup_stack(pd,this,start,
				reinterpret_cast<uintptr_t>(ec_landing_spot),stack()));
	}
	/**
	 * Creates a new GlobalThread that starts at <start> on CPU <cpu> with a custom event-base.
	 *
	 * @param start the entry-point of the Thread
	 * @param cpu the CPU to bind the Thread to
	 * @param event_base the event-base to use for this Thread
	 * @param pd the protection-domain (default: Pd::current())
	 * @param utcb the utcb-address (0 = select it automatically)
	 */
	explicit GlobalThread(startup_func start,cpu_t cpu,capsel_t event_base,Pd *pd = Pd::current(),
			uintptr_t utcb = 0) : Thread(cpu,event_base,INVALID,0,utcb) {
		create(pd,Syscalls::EC_GLOBAL,ExecEnv::setup_stack(pd,this,start,
				reinterpret_cast<uintptr_t>(ec_landing_spot),stack()));
	}

private:
	static GlobalThread _cur;
};

}
