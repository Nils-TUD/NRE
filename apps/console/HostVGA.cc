/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#include "HostVGA.h"

using namespace nul;

void HostVGA::set_page(uint uid,uint page) {
	assert(page < PAGE_COUNT);
	current(uid);
	_page = page;
	// due to odd/even addressing
	page <<= 11;
	write(START_ADDR_HI,static_cast<uint8_t>(page >> 8));
	write(START_ADDR_LO,static_cast<uint8_t>(page));
}
