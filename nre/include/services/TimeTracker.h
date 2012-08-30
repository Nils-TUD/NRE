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

#include <kobj/GlobalThread.h>
#include <kobj/VCpu.h>
#include <ipc/ClientSession.h>
#include <ipc/Connection.h>
#include <utcb/UtcbFrame.h>
#include <String.h>
#include <Desc.h>

namespace nre {

class TimeTracker {
public:
	enum Command {
		START,
		STOP
	};
};

/**
 * This service tracks the time of Scs that have been sent to him. It is primarily intended for
 * the root task to track all Scs in the system. But it can be used by other tasks as well.
 */
class TimeTrackerSession : public ClientSession {
public:
	/**
	 * Creates a new session with given connection
	 *
	 * @param con the connection
	 */
	explicit TimeTrackerSession(Connection &con) : ClientSession(con), _pts(new Pt*[CPU::count()]) {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			_pts[cpu] = con.available_on(cpu) ? new Pt(caps() + cpu) : 0;
	}
	/**
	 * Destroys this session
	 */
	virtual ~TimeTrackerSession() {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			delete _pts[cpu];
		delete[] _pts;
	}

	/**
	 * Starts the given Sc
	 */
	void start(const String &name,cpu_t cpu,capsel_t sc,bool idle) {
		UtcbFrame uf;
		uf.delegate(sc);
		uf << TimeTracker::START << name << cpu << idle;
		Pt pt(caps() + CPU::current().log_id());
		pt.call(uf);
		uf.check_reply();
	}

	/**
	 * Stops the Sc with given selector
	 */
	void stop(cpu_t cpu,capsel_t sc) {
		UtcbFrame uf;
		uf.translate(sc);
		uf << TimeTracker::STOP << cpu;
		Pt pt(caps() + CPU::current().log_id());
		pt.call(uf);
		uf.check_reply();
	}

private:
	Pt **_pts;
};

}
