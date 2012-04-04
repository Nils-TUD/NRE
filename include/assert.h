/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <ex/AssertException.h>

#ifndef NDEBUG

#	define assert(cond) do { if(!(cond)) { \
			throw AssertException(#cond,__FILE__,__LINE__); \
		} } while(0);

#else

#	define assert(cond)

#endif
