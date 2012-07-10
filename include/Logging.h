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
		if(nul::Logging::level & (lvl)) { \
			ScopedLock<nul::UserSm> guard(&nul::Logging::sm); \
			(expr); \
		} \
	} \
	while(0);

namespace nul {

class Logging {
public:
	enum {
		CHILD_CREATE	= 1 << 0,
		PFS				= 1 << 1,
		MEM_MAP			= 1 << 2,
		CPUS			= 1 << 3,
		RESOURCES		= 1 << 4,
		DATASPACES		= 1 << 5,
		SERVICES		= 1 << 6,
		CHILD_KILL		= 1 << 7,
		PFS_DETAIL		= 1 << 8,
	};

	static UserSm sm;
	static const int level = 0 |
#ifndef NDEBUG
		CHILD_CREATE | MEM_MAP | CPUS | CHILD_KILL
#else
		0
#endif
	;

private:
	Logging();
};

}
