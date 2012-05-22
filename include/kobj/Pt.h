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
#include <kobj/ObjCap.h>
#include <kobj/LocalEc.h>
#include <utcb/UtcbFrame.h>
#include <Compiler.h>
#include <Syscalls.h>

namespace nul {

class Pt : public ObjCap {
public:
	typedef ExecEnv::portal_func portal_func;

	Pt(capsel_t pt) : ObjCap(pt) {
	}
	Pt(LocalEc *ec,portal_func func,unsigned mtd = 0) : ObjCap() {
		CapHolder pt;
		Syscalls::create_pt(pt.get(),ec->sel(),reinterpret_cast<uintptr_t>(func),mtd,Pd::current()->sel());
		sel(pt.release());
	}
	Pt(LocalEc *ec,capsel_t pt,portal_func func,unsigned mtd = 0) : ObjCap(pt) {
		Syscalls::create_pt(pt,ec->sel(),reinterpret_cast<uintptr_t>(func),mtd,Pd::current()->sel());
	}
	virtual ~Pt() {
	}

	void call(UtcbFrame &uf) {
		Syscalls::call(sel());
		uf._upos = 0;
		uf._tpos = 0;
	}
};

}
