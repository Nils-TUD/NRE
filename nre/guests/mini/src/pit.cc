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

#include "pit.h"
#include "ports.h"
#include "util.h"

void PIT::init() {
	/* change timer divisor */
	uint freq = FREQ / 200;
	Ports::out<uint8_t>(CTRL,CTRL_CHAN0 | CTRL_RWLOHI | CTRL_MODE2 | CTRL_CNTBIN16);
	Ports::out<uint8_t>(CHAN0DIV,freq & 0xFF);
	Ports::out<uint8_t>(CHAN0DIV,freq >> 8);
}
