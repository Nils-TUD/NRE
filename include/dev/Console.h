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
#include <service/Consumer.h>
#include <stream/IStream.h>
#include <stream/OStream.h>
#include <dev/Keyboard.h>
#include <dev/Screen.h>
#include <mem/DataSpace.h>

namespace nul {

class Console {
public:
	enum {
		COLS	= Screen::COLS,
		ROWS	= Screen::ROWS,
	};

	enum Command {
		WRITE,
		SCROLL
	};

	struct SendPacket {
		Command cmd;
		uint8_t view;
		uint8_t x;
		uint8_t y;
		uint8_t character;
		uint8_t color;
	};

	struct ReceivePacket {
		uint8_t keycode;
		uint8_t flags;
		char character;
	};
};

class ConsoleSession : public Session {
	enum {
		IN_DS_SIZE	= ExecEnv::PAGE_SIZE,
		OUT_DS_SIZE	= ExecEnv::PAGE_SIZE,
	};

public:
	explicit ConsoleSession(Connection &con) : Session(con),
			_in_ds(IN_DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
			_out_ds(OUT_DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
			_consumer(&_in_ds,true), _producer(&_out_ds,true,true) {
		_in_ds.share(*this);
		_out_ds.share(*this);
	}

	Consumer<Console::ReceivePacket> &consumer() {
		return _consumer;
	}
	Producer<Console::SendPacket> &producer() {
		return _producer;
	}

private:
	DataSpace _in_ds;
	DataSpace _out_ds;
	Consumer<Console::ReceivePacket> _consumer;
	Producer<Console::SendPacket> _producer;
};

}
