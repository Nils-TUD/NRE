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
#include <dev/Keyboard.h>
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

	Consumer<Keyboard::Packet> &consumer() {
		return _consumer;
	}
	Producer<Console::Packet> &producer() {
		return _producer;
	}

private:
	DataSpace _in_ds;
	DataSpace _out_ds;
	Consumer<Keyboard::Packet> _consumer;
	Producer<Console::Packet> _producer;
};

}
