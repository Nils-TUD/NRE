/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <ec/Ec.h>

class GlobalEc : public Ec {
public:
	typedef void (*func_type)();

	GlobalEc(func_type start,cpu_t cpu) : Ec(cpu) {
	}
};
