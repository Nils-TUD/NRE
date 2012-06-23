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

#include <stream/Serial.h>
#include <service/Connection.h>
#include <service/Session.h>
#include <service/Producer.h>
#include <mem/DataSpace.h>
#include <kobj/Ports.h>

namespace nul {

Serial Serial::_inst INIT_PRIO_SERIAL;

Serial::Serial() : OStream(), _ports(0), _con(), _sess(), _ds(), _prod() {
	if(_startup_info.child) {
		try {
			_con = new Connection("log");
			_sess = new Session(*_con);
			_ds = new DataSpace(DS_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW);
			_prod = new Producer<char>(_ds,true);
			_ds->share(*_sess);
			return;
		}
		catch(...) {
			// ignore it when it fails. this happens for the log-service, which of course can't
			// connect to itself ;)
		}
	}

	_ports = new Ports(port,6);
	_ports->out<uint8_t>(0x80,LCR);		// Enable DLAB (set baud rate divisor)
	_ports->out<uint8_t>(0x01,DLR_LO);	// Set divisor to 1 (lo byte) 115200 baud
	_ports->out<uint8_t>(0x00,DLR_HI);	//                  (hi byte)
	_ports->out<uint8_t>(0x03,LCR);		// 8 bits, no parity, one stop bit
	_ports->out<uint8_t>(0,IER);			// disable interrupts
	_ports->out<uint8_t>(7,FCR);
	_ports->out<uint8_t>(3,MCR);
}

Serial::~Serial() {
	delete _prod;
	_prod = 0;
	delete _ds;
	_ds = 0;
	delete _sess;
	_sess = 0;
	delete _con;
	_con = 0;
	delete _ports;
	_ports = 0;
}

void Serial::write(char c) {
	if(c == '\0' || (!_ports && !_prod))
		return;

	if(_ports) {
		if(c == '\n')
			write('\r');

		while((_ports->in<uint8_t>(5) & 0x20) == 0)
			;
		_ports->out<uint8_t>(c,0);
	}
	else
		_prod->produce(c);
}

}
