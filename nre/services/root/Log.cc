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

#include <ipc/Service.h>
#include <ipc/Consumer.h>
#include <stream/Serial.h>
#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <String.h>

#include "Log.h"

using namespace nre;

Log Log::_inst INIT_PRIO_SERIAL;
Service *Log::_srv;
const char *Log::_colors[] = {
	"31","32","33","34","35","36"
};

Log::Log() : BaseSerial(), _ports(PORT_BASE,6), _sm(1), _to_ser(false), _bufpos(0), _buf() {
	_ports.out<uint8_t>(0x80,LCR);			// Enable DLAB (set baud rate divisor)
	_ports.out<uint8_t>(0x01,DLR_LO);		// Set divisor to 1 (lo byte) 115200 baud
	_ports.out<uint8_t>(0x00,DLR_HI);		//                  (hi byte)
	_ports.out<uint8_t>(0x03,LCR);			// 8 bits, no parity, one stop bit
	_ports.out<uint8_t>(0,IER);			// disable interrupts
	_ports.out<uint8_t>(7,FCR);
	_ports.out<uint8_t>(3,MCR);
	BaseSerial::_inst = this;
}

void Log::reg() {
	_srv = new Service("log",portal);
	for(CPU::iterator it = CPU::begin(); it != CPU::end(); ++it)
		_srv->provide_on(it->log_id());
	_srv->reg();
}

void Log::out(char c) {
	if(c == '\n')
		out('\r');
	while((_ports.in<uint8_t>(5) & 0x20) == 0)
		;
	_ports.out<uint8_t>(c,0);
}

void Log::buffer(char c) {
	if(_bufpos == sizeof(_buf) || c == '\n') {
		write(ROOT_SESS,_buf,_bufpos);
		_bufpos = 0;
	}
	if(c != '\n')
		_buf[_bufpos++] = c;
}

void Log::write(uint sessid,const char *line,size_t len) {
	ScopedLock<UserSm> guard(&_sm);
	_to_ser = true;
	*this << "\e[0;" << _colors[sessid % ARRAY_SIZE(_colors)] << "m[" << sessid << "] ";
	for(size_t i = 0; i < len; ++i) {
		char c = line[i];
		if(c != '\n')
			*this << c;
	}
	*this << "\e[0m\n";
	_to_ser = false;
}

void Log::portal(capsel_t pid) {
	ScopedLock<RCULock> guard(&RCU::lock());
	SessionData *sess = _srv->get_session<SessionData>(pid);
	UtcbFrameRef uf;
	try {
		String line;
		uf >> line;
		uf.finish_input();

		Log::get().write(sess->id() + 1,line.str(),line.length());
		uf << E_SUCCESS;
	}
	catch(const Exception &e) {
		uf.clear();
		uf << e.code();
	}
}
