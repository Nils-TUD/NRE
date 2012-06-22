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

void HostVGA::paint(const nul::Screen::Packet &pk) {
	if(pk.paint.x < COLS && pk.paint.y < ROWS) {
		char *screen = reinterpret_cast<char*>(_ds.virt()) + _page * ExecEnv::PAGE_SIZE;
		char *pos = screen + pk.paint.y * COLS * 2 + pk.paint.x * 2;
		*pos = pk.paint.character;
		pos++;
		*pos = pk.paint.color;
	}
}

void HostVGA::scroll() {
	char *screen = reinterpret_cast<char*>(_ds.virt()) + _page * ExecEnv::PAGE_SIZE;
	memmove(screen,screen + COLS * 2,COLS * (ROWS - 1) * 2);
	memset(screen + COLS * (ROWS - 1) * 2,0,COLS * 2);
}

void HostVGA::set_page(uint page) {
	page &= PAGE_COUNT - 1;
	_page = page;
	// due to odd/even addressing
	page <<= 11;
	write(START_ADDR_HI,static_cast<uint8_t>(page >> 8));
	write(START_ADDR_LO,static_cast<uint8_t>(page));
}
