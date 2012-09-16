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

#include <ipc/ClientSession.h>
#include <ipc/Consumer.h>
#include <mem/DataSpace.h>

namespace nre {

/**
 * Types for the vmmanager-service
 */
class VMManager {
public:
	enum Command {
		RESET,
		TERMINATE,
		KILL,
	};

	struct Packet {
		Command cmd;
	};
};

/**
 * Represents a session at the vmmanager service. This is intended for controlling vancouver
 * from the vmmanager. I.e. vmmanager provides this service and vancouver uses it and listens
 * for requests.
 */
class VMManagerSession : public ClientSession {
	enum {
		DS_SIZE	= ExecEnv::PAGE_SIZE
	};

public:
	/**
	 * Creates a new session with given connection
	 *
	 * @param con the connection
	 */
	explicit VMManagerSession(Connection &con)
			: ClientSession(con), _ds(DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW),
			  _consumer(&_ds,true) {
		create();
	}

	/**
	 * @return the consumer to receive commands from the vmmanager
	 */
	Consumer<VMManager::Packet> &consumer() {
		return _consumer;
	}

private:
	void create() {
		UtcbFrame uf;
		uf.delegate(_ds.sel());
		uf.translate(Pd::current()->sel());
		Pt pt(caps() + CPU::current().log_id());
		pt.call(uf);
		uf.check_reply();
	}

	DataSpace _ds;
	Consumer<VMManager::Packet> _consumer;
};

}
