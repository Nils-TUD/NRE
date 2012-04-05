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

class LocalEc : public Ec {
public:
	typedef void (*portal_func)(unsigned) __attribute__((regparm(1)));

	LocalEc(cpu_t cpu,cap_t event_base = 0) : Ec(cpu,Pd::current(),event_base) {
		create(Syscalls::EC_WORKER,create_stack());
	}
	virtual ~LocalEc() {
	}

	cap_t create_portal(portal_func func,unsigned mtd) {
		CapHolder ptcap(pd()->obj());
		Syscalls::create_pt(ptcap.get(),cap(),reinterpret_cast<uintptr_t>(func),mtd,pd()->cap());
		return ptcap.release();
	}
	void create_portal_for(cap_t pt,portal_func func,unsigned mtd) {
		Syscalls::create_pt(pt,cap(),reinterpret_cast<uintptr_t>(func),mtd,pd()->cap());
	}

private:
	void *create_stack();
};
