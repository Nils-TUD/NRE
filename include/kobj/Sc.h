/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <kobj/ObjCap.h>
#include <kobj/GlobalEc.h>
#include <kobj/Pd.h>
#include <cap/CapHolder.h>
#include <Syscalls.h>

namespace nul {

/**
 * Represents a scheduling context. A Sc can be bound to a GlobalEc to run it with a specified
 * priority and time quantum.
 */
class Sc : public ObjCap {
public:
	/**
	 * Creates a new Sc that is bound to the given GlobalEc. Note that it does NOT start it. Please
	 * call start() afterwards.
	 *
	 * @param ec the GlobalEc to bind it to
	 * @param qpd the quantum-priority descriptor for the Sc
	 * @param pd the pd to create it in
	 */
	explicit Sc(GlobalEc *ec,Qpd qpd,Pd *pd = Pd::current()) : ObjCap(), _ec(ec), _qpd(qpd), _pd(pd) {
		// don't create the Sc here, because then we have no chance to store the created object
		// somewhere to make it accessible for the just started Ec
	}

	/**
	 * @return the ec it is bound to
	 */
	GlobalEc *ec() {
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
	 * Starts the Sc, i.e. the attached GlobalEc.
	 */
	void start() {
		CapHolder ch;
		// in this case we should assign the selector before it has been successfully created
		// because the Sc starts immediatly. therefore, it should be completely initialized before
		// its started.
		try {
			sel(ch.get());
			Syscalls::create_sc(ch.get(),_ec->sel(),_qpd,_pd->sel());
			ch.release();
		}
		catch(...) {
			sel(INVALID);
		}
	}

private:
	Sc(const Sc&);
	Sc& operator=(const Sc&);

	GlobalEc *_ec;
	Qpd _qpd;
	Pd *_pd;
};

}
