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

#include <ipc/Session.h>
#include <ipc/Connection.h>
#include <utcb/UtcbFrame.h>
#include <kobj/Pt.h>

namespace nre {

class LogSession : public Session {
public:
	explicit LogSession(Connection &con) : Session(con), _pts(new Pt*[CPU::count()]) {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			_pts[cpu] = con.available_on(cpu) ? new Pt(caps() + cpu) : 0;
	}
	virtual ~LogSession() {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			delete _pts[cpu];
		delete[] _pts;
	}

	void write(const String &line) {
		UtcbFrame uf;
		uf << line;
		_pts[CPU::current().log_id()]->call(uf);
	}

private:
	Pt **_pts;
};

}
