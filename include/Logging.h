/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
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
