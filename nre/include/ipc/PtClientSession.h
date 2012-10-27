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

#include <ipc/ClientSession.h>

namespace nre {

/**
 * Convenience class that is responsible for managing the portals of a session. This is needed
 * for nearly every service, so that it makes sense to put it in a base class.
 */
class PtClientSession : public ClientSession {
public:
	/**
	 * Creates the session and creates portal-instances on all CPUs the service is available
	 *
	 * @param con the connection
	 */
	explicit PtClientSession(Connection &con) : ClientSession(con), _pts(new Pt *[CPU::count()]) {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			_pts[cpu] = con.available_on(cpu) ? new Pt(caps() + cpu) : 0;
	}
	/**
	 * Destroys this session
	 */
	virtual ~PtClientSession() {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			delete _pts[cpu];
		delete[] _pts;
	}

	/**
	 * @param cpu the logical cpu id (the current by default)
	 * @return the portal for the given cpu
	 */
	Pt &pt(cpu_t cpu = CPU::current().log_id()) const {
		assert(_pts[cpu] != 0);
		return *_pts[cpu];
	}

private:
	Pt **_pts;
};

}
