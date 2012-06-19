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
#include <service/Session.h>
#include <service/Connection.h>
#include <service/Producer.h>
#include <mem/DataSpace.h>

namespace nul {

class Console {
public:
	struct Packet {
		uint8_t x;
		uint8_t y;
		uint8_t character;
		uint8_t color;
	};
};

class ConsoleSession : public Session {
	enum {
		DS_SIZE = ExecEnv::PAGE_SIZE
	};

public:
	ConsoleSession(Connection &con) : Session(con),
			_ds(DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW), _producer(&_ds,true) {
		_ds.share(*this);
	}

	Producer<Console::Packet> &producer() {
		return _producer;
	}

private:
	DataSpace _ds;
	Producer<Console::Packet> _producer;
};

}
