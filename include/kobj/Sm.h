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

#include <kobj/ObjCap.h>
#include <kobj/Pd.h>
#include <arch/SyscallABI.h>

namespace nul {

class Sm : public ObjCap {
public:
	// TODO get rid of the bool
	Sm(capsel_t cap,bool) : ObjCap(cap,KEEP_CAP_BIT | KEEP_SEL_BIT) {
	}
	Sm(capsel_t cap,unsigned initial,Pd *pd = Pd::current()) : ObjCap(cap,KEEP_SEL_BIT) {
		Syscalls::create_sm(cap,initial,pd->sel());
	}
	Sm(unsigned initial,Pd *pd = Pd::current());
	virtual ~Sm() {
	}

	void down() {
		Syscalls::sm_ctrl(sel(),Syscalls::SM_DOWN);
	}
	void zero() {
		Syscalls::sm_ctrl(sel(),Syscalls::SM_ZERO);
	}
	void up() {
		Syscalls::sm_ctrl(sel(),Syscalls::SM_UP);
	}
};

}
