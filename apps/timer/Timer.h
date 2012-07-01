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

#include <arch/Types.h>
#include <kobj/Gsi.h>

class Timer {
public:
	typedef uint64_t timevalue_t;

	Timer() {
	}
	virtual ~Timer() {
	}

	virtual nul::Gsi &gsi() = 0;
	virtual void init(cpu_t cpu) = 0;
	virtual void program_timeout(uint64_t next) = 0;
};
