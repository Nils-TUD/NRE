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

#pragma once

#include <arch/Types.h>
#include <Compiler.h>

#define INIT_PRIO(X)		INIT_PRIORITY(1000 + (X))
#define INIT_PRIO_SYS(X)	INIT_PRIORITY(101 + (X))

#define INIT_PRIO_GEC		INIT_PRIO_SYS(0)
#define INIT_PRIO_PD		INIT_PRIO_SYS(1)
#define INIT_PRIO_CAPSPACE	INIT_PRIO_SYS(2)
#define INIT_PRIO_LOGGING	INIT_PRIO_SYS(2)
#define INIT_PRIO_RCU		INIT_PRIO_SYS(3)
#define INIT_PRIO_CPUS		INIT_PRIO_SYS(4)
#define INIT_PRIO_VMEM		INIT_PRIO_SYS(5)
#define INIT_PRIO_PMEM		INIT_PRIO_SYS(6)
#define INIT_PRIO_HV		INIT_PRIO_SYS(7)
#define INIT_PRIO_CPU0		INIT_PRIO_SYS(8)
#define INIT_PRIO_SERIAL	INIT_PRIO_SYS(9)

namespace nre {

class Utcb;
class Hip;

struct StartupInfo {
	Hip *hip;
	uintptr_t utcb;
	uintptr_t stack;
	word_t cpu;
	word_t done;
	word_t child;
};

}

EXTERN_C void _post_init();
extern nre::StartupInfo _startup_info;
