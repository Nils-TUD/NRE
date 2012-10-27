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
#include <kobj/Pd.h>
#include <util/ScopedCapSels.h>
#include <Syscalls.h>
#include <CPU.h>

namespace nre {

class Sc;

/**
 * Represents a virtual CPU. Note that you have to call start() to bind it to an Sc and start it.
 */
class VCpu : public Ec {
public:
	/**
	 * Constructor
	 *
	 * @param cpu the logical cpu to bind the VCPU to
	 * @param evb the offset for the event-portals
	 * @param cap the capability (INVALID if a new one should be used)
	 * @param name the name of the vcpu (only used for display-purposes)
	 */
	explicit VCpu(cpu_t cpu, capsel_t evb, const String &name)
		: Ec(cpu, evb, INVALID), _sc(), _name(name) {
		ScopedCapSels cap;
		Syscalls::create_ec(cap.get(), 0, 0, CPU::get(cpu).phys_id(), evb,
		                    Syscalls::EC_GLOBAL, Pd::current()->sel());
		sel(cap.release());
	}
	virtual ~VCpu();

	/**
	 * @return the scheduling context this vcpu is bound to
	 */
	Sc *sc() const {
		return _sc;
	}
	/**
	 * @return the name of this vcpu
	 */
	const String &name() const {
		return _name;
	}

	/**
	 * Starts this vcpu with given quantum-priority-descriptor
	 *
	 * @param qpd the qpd to use
	 */
	void start(Qpd qpd = Qpd());

private:
	Sc *_sc;
	const String _name;
};

}
