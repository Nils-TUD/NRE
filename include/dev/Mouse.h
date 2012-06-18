/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <arch/Types.h>
#include <service/Connection.h>
#include <service/Session.h>
#include <service/Consumer.h>
#include <mem/DataSpace.h>

namespace nul {

class Mouse {
	enum {
		DS_SIZE	= ExecEnv::PAGE_SIZE
	};

public:
	struct Packet {
		uint8_t status;
		uint8_t x;
		uint8_t y;
		uint8_t z;
	};

	Mouse() : _con("mouse"), _sess(_con), _ds(DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
			_consumer(&_ds,true) {
		_ds.share(_sess);
	}

	Consumer<Packet> &consumer() {
		return _consumer;
	}

private:
	Connection _con;
	Session _sess;
	DataSpace _ds;
	Consumer<Packet> _consumer;
};

}