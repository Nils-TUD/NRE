/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <arch/Types.h>
#include <dev/Keyboard.h>

class Keymap {
	struct KeymapEntry {
		char def;
		char shift;
		char alt;
	};

public:
	static char translate(const nul::Keyboard::Packet &in) {
		KeymapEntry *km = keymap + in.keycode;
		if(in.flags & (nul::Keyboard::LSHIFT | nul::Keyboard::RSHIFT))
			return km->shift;
		if(in.flags & (nul::Keyboard::LALT | nul::Keyboard::RALT))
			return km->alt;
		return km->def;
	}

private:
	Keymap();

	static KeymapEntry keymap[];
};
