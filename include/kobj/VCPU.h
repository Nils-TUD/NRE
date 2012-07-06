/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <kobj/Ec.h>
#include <util/ScopedCapSels.h>
#include <Syscalls.h>

namespace nul {

class VCPU : public Ec {
public:
	/**
	 * Constructor
	 *
	 * @param cpu the logical cpu to bind the VCPU to
	 * @param evb the offset for the event-portals
	 * @param cap the capability (INVALID if a new one should be used)
	 */
	explicit VCPU(cpu_t cpu,capsel_t evb) : Ec(cpu,evb,INVALID) {
		ScopedCapSels cap;
		Syscalls::create_ec(cap.get(),0,0,cpu,evb,Syscalls::EC_GLOBAL,Pd::current()->sel());
		sel(cap.release());
	}
};

}
