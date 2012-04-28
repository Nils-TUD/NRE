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

#include <format/Log.h>
#include <Ports.h>

namespace nul {

Log::Log() : Format() {
	Ports::out<uint8_t>(port + 1,0x00); // Disable all interrupts
	Ports::out<uint8_t>(port + 3,0x80); // Enable DLAB (set baud rate divisor)
	Ports::out<uint8_t>(port + 0,0x03); // Set divisor to 3 (lo byte) 38400 baud
	Ports::out<uint8_t>(port + 1,0x00); //                  (hi byte)
	Ports::out<uint8_t>(port + 3,0x03); // 8 bits, no parity, one stop bit
	Ports::out<uint8_t>(port + 2,0xC7); // Enable FIFO, clear them, with 14-byte threshold
	Ports::out<uint8_t>(port + 4,0x0B); // IRQs enabled, RTS/DSR set
}

void Log::printc(char c) {
	while((Ports::in<uint8_t>(port + 5) & 0x20) == 0)
		;
	Ports::out<uint8_t>(port,c);
}

}
