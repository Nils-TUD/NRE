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
#include <kobj/KObject.h>
#include <kobj/LocalEc.h>
#include <utcb/UtcbFrame.h>
#include <Compiler.h>
#include <Syscalls.h>

namespace nul {

class Pt : public KObject {
public:
	typedef ExecEnv::portal_func portal_func;

	Pt(LocalEc *ec,portal_func func,unsigned mtd = 0) : KObject() {
		// TODO use a specific cap-range for portals, so that we can provide per-portal-data with
		// a simple array (or at least for some portals). we will need that for pagefault-portals
		// for example because we need a way to find the client that caused it.
		Pd *pd = Pd::current();
		CapHolder ptcap;
		Syscalls::create_pt(ptcap.get(),ec->cap(),reinterpret_cast<uintptr_t>(func),mtd,pd->cap());
		cap(ptcap.release());
	}
	Pt(LocalEc *ec,cap_t pt,portal_func func,unsigned mtd = 0) : KObject(pt) {
		Pd *pd = Pd::current();
		Syscalls::create_pt(pt,ec->cap(),reinterpret_cast<uintptr_t>(func),mtd,pd->cap());
	}

	void call(UtcbFrame &uf) {
		Syscalls::call(cap());
		uf._rpos = 0;
	}

	void call() {
		Syscalls::call(cap());
		//uf._rpos = 0;
	}
};

}
