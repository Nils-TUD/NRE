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

#include <kobj/KObject.h>
#include <kobj/GlobalEc.h>
#include <kobj/Pd.h>
#include <cap/CapHolder.h>
#include <Syscalls.h>

namespace nul {

class Sc : public KObject {
public:
	explicit Sc(GlobalEc *ec,Qpd qpd,Pd *pd = Pd::current()) : KObject() {
		CapHolder ch;
		Syscalls::create_sc(ch.get(),ec->cap(),qpd,pd->cap());
		cap(ch.release());
	}
	virtual ~Sc() {
	}

private:
	Sc(const Sc&);
	Sc& operator=(const Sc&);
};

}
