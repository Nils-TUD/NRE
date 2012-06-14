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
#include <kobj/Ports.h>

namespace nul {

Serial Serial::_inst;

void Serial::init() {
	_ports = new Ports(port,6);
	_ports->out<uint8_t>(LCR,0x80);		// Enable DLAB (set baud rate divisor)
	_ports->out<uint8_t>(DLR_LO,0x01);	// Set divisor to 1 (lo byte) 115200 baud
	_ports->out<uint8_t>(DLR_HI,0x00);	//                  (hi byte)
	_ports->out<uint8_t>(LCR,0x03);		// 8 bits, no parity, one stop bit
	_ports->out<uint8_t>(IER,0);			// disable interrupts
	_ports->out<uint8_t>(FCR,7);
	_ports->out<uint8_t>(MCR,3);
}

void Serial::write(char c) {
	if(c == '\0' || !_ports)
		return;

	if(c == '\n')
		write('\r');

	while((_ports->in<uint8_t>(5) & 0x20) == 0)
		;
	_ports->out<uint8_t>(0,c);
}

}
