/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
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
