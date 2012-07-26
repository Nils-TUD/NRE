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

#include <arch/Types.h>
#include <ipc/Connection.h>
#include <ipc/ClientSession.h>
#include <utcb/UtcbFrame.h>
#include <Exception.h>
#include <CPU.h>

namespace nre {

class RebootSession : public ClientSession {
public:
	explicit RebootSession(Connection &con) : ClientSession(con), _pts(new Pt*[CPU::count()]) {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			_pts[cpu] = con.available_on(cpu) ? new Pt(caps() + cpu) : 0;
	}
	virtual ~RebootSession() {
		for(cpu_t cpu = 0; cpu < CPU::count(); ++cpu)
			delete _pts[cpu];
		delete[] _pts;
	}

	void reboot() {
		UtcbFrame uf;
		_pts[CPU::current().log_id()]->call(uf);
		uf.check_reply();
	}

private:
	Pt **_pts;
};

}
