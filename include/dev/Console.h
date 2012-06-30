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
#include <kobj/Pt.h>
#include <service/Session.h>
#include <service/Connection.h>
#include <dev/Keyboard.h>
#include <Hip.h>

namespace nul {

class Console {
public:
	enum {
		COLS		= 80,
		ROWS		= 25,
		BUF_SIZE	= COLS * 2 + 1
	};

	enum ViewCommand {
		CREATE_VIEW,
		DESTROY_VIEW
	};

	struct ReceivePacket {
		uint flags;
		uint8_t keycode;
		char character;
	};
};

class ConsoleSession : public Session {
public:
	explicit ConsoleSession(Connection &con) : Session(con), _pts() {
		for(cpu_t cpu = 0; cpu < Hip::MAX_CPUS; ++cpu) {
			if(con.available_on(cpu))
				_pts[cpu] = new Pt(caps() + cpu);
		}
	}

	Pt &pt(cpu_t cpu) const {
		assert(_pts[cpu] != 0);
		return *_pts[cpu];
	}

private:
	Pt *_pts[Hip::MAX_CPUS];
};

}
