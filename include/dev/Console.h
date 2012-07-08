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
		TAB_WIDTH	= 4,
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
	explicit ConsoleSession(Connection &con) : Session(con), _pts(new Pt*[CPU::count()]) {
		for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
			_pts[it->log_id()] = con.available_on(it->log_id()) ? new Pt(caps() + it->log_id()) : 0;
	}
	virtual ~ConsoleSession() {
		for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
			delete _pts[it->log_id()];
		delete[] _pts;
	}

	Pt &pt(cpu_t cpu) const {
		assert(_pts[cpu] != 0);
		return *_pts[cpu];
	}

private:
	Pt **_pts;
};

}
