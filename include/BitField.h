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
#include <cstring>

namespace nul {

template<uint BITS>
class BitField {
public:
	BitField() : _words() {
	}

	bool is_set(uint bit) const {
		return (_words[bit / 8] & (1 << (bit % 8))) != 0;
	}

	void set(uint bit) {
		_words[bit / 8] |= 1 << (bit % 8);
	}
	void set(uint bit,bool value) {
		if(value)
			set(bit);
		else
			clear(bit);
	}
	void clear(uint bit) {
		_words[bit / 8] &= ~(1 << (bit % 8));
	}

	void set_all() {
		memset(_words,-1,(BITS / 8) * sizeof(word_t));
	}
	void clear_all() {
		memset(_words,0,(BITS / 8) * sizeof(word_t));
	}

private:
	word_t _words[BITS / 8];
};

}
