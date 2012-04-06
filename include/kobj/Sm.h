/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <kobj/KObject.h>

class Sm : public KObject {
public:
	Sm(unsigned initial = 0) : KObject(Pd::current()) {
		CapHolder ch(pd()->obj());
		Syscalls::create_sm(ch.get(),initial,pd()->cap());
		cap(ch.release());
	}

	void down() {
		Syscalls::sm_ctrl(cap(),Syscalls::SM_DOWN);
	}
	void zero() {
		Syscalls::sm_ctrl(cap(),Syscalls::SM_ZERO);
	}
	void up() {
		Syscalls::sm_ctrl(cap(),Syscalls::SM_UP);
	}
};
