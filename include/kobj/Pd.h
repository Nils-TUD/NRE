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
#include <kobj/ObjCap.h>
#include <Syscalls.h>

namespace nre {

class GlobalThread;

/**
 * Represents a protection domain
 */
class Pd : public ObjCap {
	friend class GlobalThread;
	friend void ::_post_init();

	/**
	 * Creates protection-domain object for the current Pd (only used during startup)
	 *
	 * @param cap the capability-selector
	 */
	explicit Pd(capsel_t cap);

public:
	/**
	 * @return the current protection domain
	 */
	static Pd *current();

	/**
	 * Creates a new protection domain
	 *
	 * @param crd the capabilities to pass to the new Pd
	 * @param pd the protection-domain to create the new Pd in
	 */
	explicit Pd(Crd crd = Crd(0),Pd *pd = Pd::current());

private:
	Pd(const Pd&);
	Pd& operator=(const Pd&);

	static Pd _cur;
};

}
