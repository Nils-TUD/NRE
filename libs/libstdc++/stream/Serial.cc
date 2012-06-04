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
#include <Ports.h>

namespace nul {

Serial Serial::_inst;

void Serial::init() {
	Ports::out<uint8_t>(port + LCR,0x80);		// Enable DLAB (set baud rate divisor)
	Ports::out<uint8_t>(port + DLR_LO,0x01);	// Set divisor to 1 (lo byte) 115200 baud
	Ports::out<uint8_t>(port + DLR_HI,0x00);	//                  (hi byte)
	Ports::out<uint8_t>(port + LCR,0x03);		// 8 bits, no parity, one stop bit
	Ports::out<uint8_t>(port + IER,0);			// disable interrupts
	Ports::out<uint8_t>(port + FCR,7);
	Ports::out<uint8_t>(port + MCR,3);
	_inited = true;
}

void Serial::write(char c) {
	if(c == '\0' || !_inited)
		return;

	if(c == '\n')
		write('\r');

	while((Ports::in<uint8_t>(port + 5) & 0x20) == 0)
		;
	Ports::out<uint8_t>(port,c);
}

}
