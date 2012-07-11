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
#include <services/Keyboard.h>

class Keymap {
	struct KeymapEntry {
		char def;
		char shift;
		char alt;
	};

public:
	static char translate(const nre::Keyboard::Packet &in) {
		KeymapEntry *km = keymap + in.keycode;
		if(in.flags & (nre::Keyboard::LSHIFT | nre::Keyboard::RSHIFT))
			return km->shift;
		if(in.flags & (nre::Keyboard::LALT | nre::Keyboard::RALT))
			return km->alt;
		return km->def;
	}

private:
	Keymap();

	static KeymapEntry keymap[];
};
