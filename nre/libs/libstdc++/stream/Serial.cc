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

#include <stream/Serial.h>
#include <ipc/Connection.h>
#include <mem/DataSpace.h>
#include <kobj/Ports.h>
#include <services/Log.h>

namespace nre {

BaseSerial *BaseSerial::_inst = 0;
Serial::Init Serial::Init::init INIT_PRIO_SERIAL;

Serial::Init::Init() {
	// only the child uses this class. since root offers the log-service in itself, it uses a
	// different sub-class.
	if(_startup_info.child)
		BaseSerial::_inst = new Serial();
}

Serial::Serial()
	: BaseSerial(), _con(new Connection("log")), _sess(new LogSession(*_con)), _bufpos(0), _buf() {
}

Serial::~Serial() {
	delete _sess;
	delete _con;
}

void Serial::write(char c) {
	if(c == '\0')
		return;

	if(_bufpos == sizeof(_buf) || c == '\n') {
		_sess->write(String(_buf,_bufpos));
		_bufpos = 0;
	}
	if(c != '\n')
		_buf[_bufpos++] = c;
}

}
