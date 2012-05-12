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

#include <Compiler.h>
#include <Types.h>

#define INIT_PRIO(X)		INIT_PRIORITY(101 + (X))
#define INIT_PRIO_PD		INIT_PRIO(0)
#define INIT_PRIO_GLOBALEC	INIT_PRIO(1)

namespace nul {

class Utcb;
class Hip;

struct StartupInfo {
	Hip *hip;
	Utcb *utcb;
	uint32_t cpu;
};

}

extern "C" void _presetup();
extern "C" void _setup(bool child);
extern nul::StartupInfo _startup_info;
