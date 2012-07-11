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

#include <kobj/ObjCap.h>
#include <kobj/GlobalThread.h>
#include <kobj/VCPU.h>
#include <kobj/Pd.h>
#include <util/ScopedCapSels.h>
#include <Syscalls.h>

namespace nre {

/**
 * Represents a scheduling context. A Sc can be bound to a GlobalThread to run it with a specified
 * priority and time quantum.
 */
class Sc : public ObjCap {
public:
	/**
	 * Creates a new Sc that is bound to the given GlobalThread. Note that it does NOT start it. Please
	 * call start() afterwards.
	 *
	 * @param ec the GlobalThread to bind it to
	 * @param qpd the quantum-priority descriptor for the Sc
	 * @param pd the pd to create it in
	 */
	explicit Sc(GlobalThread *ec,Qpd qpd,Pd *pd = Pd::current()) : ObjCap(), _ec(ec), _qpd(qpd), _pd(pd) {
		// don't create the Sc here, because then we have no chance to store the created object
		// somewhere to make it accessible for the just started Thread
	}
	/**
	 * Creates a new Sc that is bound to the given VCPU. Note that it does NOT start it. Please
	 * call start() afterwards.
	 *
	 * @param vcpu the VCPU to bind it to
	 * @param qpd the quantum-priority descriptor for the Sc
	 */
	explicit Sc(VCPU *vcpu,Qpd qpd) : ObjCap(), _ec(vcpu), _qpd(qpd), _pd(Pd::current()) {
	}

	/**
	 * @return the ec it is bound to
	 */
	Ec *ec() {
		return _ec;
	}
	/**
	 * @return the quantum-priority descriptor
	 */
	Qpd qpd() const {
		return _qpd;
	}
	/**
	 * @return the protection-domain it belongs to
	 */
	Pd *pd() {
		return _pd;
	}

	/**
	 * Starts the Sc, i.e. the attached GlobalThread.
	 */
	void start() {
		ScopedCapSels scs;
		// in this case we should assign the selector before it has been successfully created
		// because the Sc starts immediatly. therefore, it should be completely initialized before
		// its started.
		try {
			sel(scs.get());
			Syscalls::create_sc(scs.get(),_ec->sel(),_qpd,_pd->sel());
			scs.release();
		}
		catch(...) {
			sel(INVALID);
		}
	}

private:
	Sc(const Sc&);
	Sc& operator=(const Sc&);

	Ec *_ec;
	Qpd _qpd;
	Pd *_pd;
};

}
