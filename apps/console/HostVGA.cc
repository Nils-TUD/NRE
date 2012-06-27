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

void HostVGA::paint(uint uid,uint8_t x,uint8_t y,uint8_t *buffer,size_t count) {
	if(uid == current()) {
		assert((y * COLS * 2 + x * 2) + count <= (ROWS * COLS * 2));
		memcpy(reinterpret_cast<char*>(_ds.virt()) + y * COLS * 2 + x * 2,buffer,count);
	}
}

void HostVGA::set_page(uint uid,uint page) {
	current(uid);
	page &= PAGE_COUNT - 1;
	_page = page;
	// due to odd/even addressing
	page <<= 11;
	write(START_ADDR_HI,static_cast<uint8_t>(page >> 8));
	write(START_ADDR_LO,static_cast<uint8_t>(page));
}
