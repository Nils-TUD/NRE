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

#include <kobj/UserSm.h>
#include <mem/DataSpace.h>

#include "bus/motherboard.h"
#include "Timeouts.h"

class Vancouver : public StaticReceiver<Vancouver> {
public:
	Vancouver(const char *args,size_t ramsize = nul::ExecEnv::PAGE_SIZE * 1024 * 8)
			: _mb(), _timeouts(_mb),
			  _guest_mem(ramsize,nul::DataSpaceDesc::ANONYMOUS,nul::DataSpaceDesc::RWX),
			  _guest_size(ramsize), _conscon("console"), _conssess(_conscon), _console(_conssess) {
		create_devices(args);
		create_vcpus();
	}

	void reset();
	bool receive(CpuMessage &msg);
	bool receive(MessageHostOp &msg);
	bool receive(MessagePciConfig &msg);
	bool receive(MessageAcpi &msg);
	bool receive(MessageTimer &msg);
	bool receive(MessageTime &msg);
	bool receive(MessageLegacy &msg);
	bool receive(MessageConsoleView &msg);

private:
	void create_devices(const char *args);
	void create_vcpus();

	Motherboard _mb;
	Timeouts _timeouts;
	nul::DataSpace _guest_mem;
	size_t _guest_size;
	nul::Connection _conscon;
	nul::ConsoleSession _conssess;
	nul::ConsoleView _console;
};

extern nul::UserSm globalsm;
