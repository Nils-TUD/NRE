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

/**
 * Executes the given expression (which should to the logging) if the log-level <lvl> is enabled.
 * A UserSm is used to make sure that different log-statements in one program don't get mixed.
 * The nice thing is that the compiler is able to completely eliminate this code, if the log-level
 * is disabled.
 */
#define LOG(lvl,expr) 	\
	do { \
		if(nre::Logging::level & (lvl)) { \
			nre::ScopedLock<nre::UserSm> guard(&nre::Logging::sm); \
			(expr); \
		} \
	} \
	while(0);

namespace nre {

/**
 * This class is used to control debugging-output
 */
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
		ADMISSION		= 1 << 15,
		EXCEPTIONS		= 1 << 16,
		PLATFORM		= 1 << 17,
		PCI				= 1 << 18,
		STORAGE			= 1 << 19,
		STORAGE_DETAIL	= 1 << 20,
	};

	static UserSm sm;
	static const int level = 0 |
#ifndef NDEBUG
		CHILD_CREATE | MEM_MAP | CPUS | PLATFORM | CHILD_KILL | ACPI |
		REBOOT | TIMER | KEYBOARD | STORAGE
#else
		CHILD_KILL | MEM_MAP | PLATFORM | STORAGE
#endif
	;

private:
	Logging();
};

}
