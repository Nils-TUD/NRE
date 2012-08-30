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

#include <kobj/Thread.h>
#include <kobj/LocalThread.h>
#include <Compiler.h>
#include <CPU.h>
#include <RCU.h>

namespace nre {

void Thread::create(Pd *pd,Syscalls::ECType type,void *sp) {
	ScopedCapSels scs;
	Syscalls::create_ec(scs.get(),utcb(),sp,CPU::get(cpu()).phys_id(),event_base(),type,pd->sel());
	if(pd == Pd::current())
		RCU::add(this);
	sel(scs.release());
}

Thread::~Thread() {
	RCU::remove(this);
	// FIXME: it seems to be very difficult to know when localthreads can be destroyed. because
	// there is no easy way to determine whether a call is currently in progress. the only way
	// that I can see is having one globalthread running for every CPU and causing the one that
	// runs on the same CPU as the localthread to call a dummyportal that is handled by the
	// localthread. this way, we know after the call (assuming that all other portals handled
	// by that localthread have been revoked before that) that the localthread is in kernel
	// and will never leave it again. thus, we can destroy his stack (which is the most critical
	// thing). but having all these globalthreads is too much overhead and not worth it.
	// this raises the question whether the localthread+portal concept of NOVA is really a good
	// thing. because assuming that it works like in other kernels, where you do a "wait_for_msg"
	// to receive the next message and don't wait in kernel, terminating threads/services would be
	// trivial.
	delete _stack;
	delete _utcb;
}

// slot 0 is reserved
size_t Thread::_tls_idx = 1;

}
