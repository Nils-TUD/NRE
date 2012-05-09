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

	Pt(cap_t pt) : KObject(pt) {
	}
	Pt(LocalEc *ec,portal_func func,unsigned mtd = 0) : KObject() {
		CapHolder pt;
		Syscalls::create_pt(pt.get(),ec->cap(),reinterpret_cast<uintptr_t>(func),mtd,Pd::current()->cap());
		cap(pt.release());
	}
	Pt(LocalEc *ec,cap_t pt,portal_func func,unsigned mtd = 0) : KObject(pt) {
		Syscalls::create_pt(pt,ec->cap(),reinterpret_cast<uintptr_t>(func),mtd,Pd::current()->cap());
	}

	void call(UtcbFrame &uf) {
		Syscalls::call(cap());
		uf._upos = 0;
	}
};

}
