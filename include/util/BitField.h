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
class BitField;
class OStream;
template<uint BITS>
static inline OStream &operator<<(OStream &os,const BitField<BITS> &bf);

template<uint BITS>
class BitField {
	template<uint N>
	friend OStream &operator<<(OStream &os,const BitField<N> &bf);

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

	uint first_set() const {
		// TODO this can be improved
		for(uint i = 0; i < BITS; ++i) {
			if(is_set(i))
				return i;
		}
		return BITS;
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
	word_t _words[(BITS + sizeof(word_t) * 8 - 1) / (sizeof(word_t) * 8)];
};

template<uint BITS>
static inline OStream &operator<<(OStream &os,const BitField<BITS> &bf) {
	os << "Bitfield[";
	for(size_t i = 0; i < ARRAY_SIZE(bf._words); ++i) {
		os.writef("%0"FMT_WORD_BYTES"x",bf._words[i]);
		if(i + 1 < ARRAY_SIZE(bf._words))
			os << ' ';
	}
	os << ']';
	return os;
}

}
