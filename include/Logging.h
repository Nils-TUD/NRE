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
#include <kobj/UserSm.h>
#include <util/ScopedLock.h>

#define LOG(lvl,expr) 	\
	do { \
		if(nre::Logging::level & (lvl)) { \
			ScopedLock<nre::UserSm> guard(&nre::Logging::sm); \
			(expr); \
		} \
	} \
	while(0);

namespace nre {

class Logging {
public:
	enum {
		CHILD_CREATE	= 1 << 0,
		CHILD_KILL		= 1 << 1,
		PFS				= 1 << 2,
		PFS_DETAIL		= 1 << 3,
		CPUS			= 1 << 4,
		MEM_MAP			= 1 << 5,
		RESOURCES		= 1 << 6,
		DATASPACES		= 1 << 7,
		SERVICES		= 1 << 8,
		ACPI			= 1 << 9,
		PCICFG			= 1 << 10,
		TIMER			= 1 << 11,
		TIMER_DETAIL	= 1 << 12,
		REBOOT			= 1 << 13,
		KEYBOARD		= 1 << 14,
	};

	static UserSm sm;
	static const int level = 0 |
#ifndef NDEBUG
		CHILD_CREATE | MEM_MAP | CPUS | CHILD_KILL | ACPI | PCICFG | REBOOT | TIMER | KEYBOARD
#else
		0
#endif
	;

private:
	Logging();
};

}
