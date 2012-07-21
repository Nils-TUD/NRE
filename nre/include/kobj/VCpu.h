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

#include <kobj/Ec.h>
#include <util/ScopedCapSels.h>
#include <Syscalls.h>
#include <CPU.h>

namespace nre {

class VCpu : public Ec {
public:
	/**
	 * Constructor
	 *
	 * @param cpu the logical cpu to bind the VCPU to
	 * @param evb the offset for the event-portals
	 * @param cap the capability (INVALID if a new one should be used)
	 */
	explicit VCpu(cpu_t cpu,capsel_t evb) : Ec(cpu,evb,INVALID) {
		ScopedCapSels cap;
		Syscalls::create_ec(cap.get(),0,0,CPU::get(cpu).phys_id(),evb,
				Syscalls::EC_GLOBAL,Pd::current()->sel());
		sel(cap.release());
	}
};

}
