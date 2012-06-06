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
	static size_t idx(uint bit) {
		return bit / (sizeof(word_t) * 8);
	}
	static size_t bitpos(uint bit) {
		return 1 << (bit % (sizeof(word_t) * 8));
	}

public:
	BitField() : _words() {
	}

	bool is_set(uint bit) const {
		return (_words[idx(bit)] & bitpos(bit)) != 0;
	}

	void set(uint bit) {
		_words[idx(bit)] |= bitpos(bit);
	}
	void set(uint bit,bool value) {
		if(value)
			set(bit);
		else
			clear(bit);
	}
	void clear(uint bit) {
		_words[idx(bit)] &= ~bitpos(bit);
	}

	void set_all() {
		memset(_words,-1,sizeof(_words));
	}
	void clear_all() {
		memset(_words,0,sizeof(_words));
	}

private:
	word_t _words[BITS / (sizeof(word_t) * 8)];
};

}
