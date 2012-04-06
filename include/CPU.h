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

#include <Hip.h>
#include <assert.h>

class LocalEc;

class CPU {
public:
	static CPU &get(cpu_t id) {
		assert(id < Hip::MAX_CPUS);
		return cpus[id];
	}

	CPU() : id(), map_pt(), ec() {
	}

	cpu_t id;
	cap_t map_pt;
	LocalEc * ec;

private:
	static CPU cpus[Hip::MAX_CPUS];
};
