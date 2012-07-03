/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <arch/Types.h>
#include <service/Connection.h>
#include <service/Session.h>
#include <utcb/UtcbFrame.h>
#include <Exception.h>
#include <CPU.h>

namespace nul {

class RebootSession : public Session {
public:
	explicit RebootSession(Connection &con) : Session(con), _pts(new Pt*[CPU::count()]) {
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
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw Exception(res);
	}

private:
	Pt **_pts;
};

}
