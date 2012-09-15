/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2007-2009, Bernhard Kauer <bk@vmmon.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of Vancouver.
 *
 * Vancouver is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Vancouver is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <kobj/UserSm.h>
#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <mem/DataSpace.h>

#include "bus/motherboard.h"
#include "Timeouts.h"
#include "StorageDevice.h"

class Vancouver : public StaticReceiver<Vancouver> {
public:
	explicit Vancouver(const char *args,size_t subcon)
			: _lt_input(keyboard_thread,nre::CPU::current().log_id()),
			  _sc_input(&_lt_input,nre::Qpd()), _mb(), _timeouts(_mb),
			  // TODO find a good name
			  _conscon("console"), _conssess(_conscon,subcon,nre::String("VM")), _stcon(),
			  _stdevs() {
		// storage is optional
		try {
			_stcon = new nre::Connection("storage");
		}
		catch(const nre::Exception &e) {
			nre::Serial::get() << "Unable to connect to storage: " << e.msg() << "\n";
		}
		create_devices(args);
		create_vcpus();
		_lt_input.set_tls<Vancouver*>(nre::Thread::TLS_PARAM,this);
		_sc_input.start(nre::String("vmm-input"));
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
	bool receive(MessageDisk &msg);

private:
	static void keyboard_thread(void*);
	void create_devices(const char *args);
	void create_vcpus();

	nre::GlobalThread _lt_input;
	nre::Sc _sc_input;
	Motherboard _mb;
	Timeouts _timeouts;
	nre::Connection _conscon;
	nre::ConsoleSession _conssess;
	nre::Connection *_stcon;
	StorageDevice *_stdevs[nre::Storage::MAX_CONTROLLER * nre::Storage::MAX_DRIVES];
};

extern nre::UserSm globalsm;
