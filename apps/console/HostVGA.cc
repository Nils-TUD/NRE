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

void HostVGA::set_regs(const nul::Console::Register &regs) {
	if(regs.cursor_style != _last.cursor_style) {
		write(CURSOR_HI,regs.cursor_style >> 8);
		write(CURSOR_LO,regs.cursor_style);
	}
	uint16_t cursor_offset = regs.cursor_pos -  regs.offset;
	if(regs.cursor_pos != _last.cursor_pos) {
		write(CURSOR_LOC_HI,3 * 8 + (cursor_offset >> 8));
		write(CURSOR_LOC_LO,cursor_offset);
	}
	if(regs.offset != _last.offset) {
		uintptr_t offset = regs.offset - (Console::TEXT_OFF >> 1);
		write(START_ADDR_HI,static_cast<uint8_t>(offset >> 8));
		write(START_ADDR_LO,static_cast<uint8_t>(offset));
	}
	_last = regs;
}
