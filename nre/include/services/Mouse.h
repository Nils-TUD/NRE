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
#include <ipc/Consumer.h>
#include <mem/DataSpace.h>

namespace nre {

class Mouse {
public:
	struct Packet {
		uint8_t status;
		uint8_t x;
		uint8_t y;
		uint8_t z;
	};

private:
	Mouse();
};

class MouseSession : public ClientSession {
	enum {
		DS_SIZE	= ExecEnv::PAGE_SIZE
	};

public:
	explicit MouseSession(Connection &con) : ClientSession(con),
			_ds(DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW), _consumer(&_ds,true) {
		share();
	}

	Consumer<Mouse::Packet> &consumer() {
		return _consumer;
	}

private:
	void share() {
		UtcbFrame uf;
		uf.delegate(_ds.sel());
		uf << _ds.desc();
		Pt pt(caps() + CPU::current().log_id());
		pt.call(uf);
		uf.check_reply();
	}

	DataSpace _ds;
	Consumer<Mouse::Packet> _consumer;
};

}
