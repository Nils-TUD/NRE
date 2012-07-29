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

#include <kobj/LocalThread.h>
#include <kobj/Pt.h>
#include <utcb/UtcbFrame.h>

namespace nre {

LocalThread::~LocalThread() {
	// this assumes that we don't call this from our own thread, of course
	assert(Thread::current() != this);
	// first make sure that we're not in a portal-call.
	// this assumes that all portals handled by this thread are destroyed _before_ this thread
	UtcbFrame uf;
	Pt pt(this,portal);
	pt.call(uf);
}

}
