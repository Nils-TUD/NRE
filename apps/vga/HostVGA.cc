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

void HostVGA::put(const nul::Console::Packet &pk) {
	if(pk.x < COLS && pk.y < ROWS) {
		char *screen = reinterpret_cast<char*>(_ds.virt()) + _page * ExecEnv::PAGE_SIZE;
		char *pos = screen + pk.y * COLS * 2 + pk.x * 2;
		*pos = pk.character;
		pos++;
		*pos = pk.color;
	}
}
