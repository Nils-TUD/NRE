/*
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <arch/Types.h>
#include <arch/Defines.h>
#include <stream/OStream.h>
#include <util/Math.h>
#include <Compiler.h>
#include <cstring>
#include <Assert.h>

namespace nre {

template<uint BITS>
class MaskField;
template<uint BITS>
static inline OStream &operator<<(OStream &os,const MaskField<BITS> &bf);

/**
 * MaskField is essentially the same as BitField but allows multiple bits. That is, it manages
 * X bit masks of BITS bits and you can get and set individual masks.
 */
template<uint BITS>
class MaskField {
	template<uint N>
	friend OStream &operator<<(OStream &os,const MaskField<N> &bf);

	static const word_t MASK = (1 << BITS) - 1;

	static size_t idx(uint i) {
		return (i * BITS) / (sizeof(word_t) * 8);
	}
	static uint obj(uint i) {
		return i % ((sizeof(word_t) * 8) / BITS);
	}

public:
	/**
	 * Creates a new maskfield with <total> bits
	 *
	 * @param total the total number of bits
	 */
	explicit MaskField(size_t total)
			: _bits(total), _count(Math::blockcount<size_t>(total,(sizeof(word_t) * 8))),
			  _words(new word_t[_count]) {
		STATIC_ASSERT((BITS & (BITS - 1)) == 0);
	}
	~MaskField() {
		delete[] _words;
	}

	/**
	 * Extracts the mask with index <i>
	 *
	 * @param i the index of the mask (starting with 0)
	 * @return the mask
	 */
	word_t get(size_t i) const {
		assert(i < _bits / BITS);
		return (_words[idx(i)] >> (BITS * obj(i))) & MASK;
	}
	/**
	 * Sets the mask <i> to <mask>
	 *
	 * @param i the index of the mask (starting with 0)
	 * @param mask the new value
	 */
	void set(size_t i,word_t mask) {
		assert(i < _bits / BITS);
		size_t id = idx(i);
		uint ob = obj(i);
		_words[id] = (_words[id] & ~(MASK << (BITS * ob))) | ((mask & MASK) << (BITS * ob));
	}

	/**
	 * Sets all bits to 1
	 */
	void set_all() {
		memset(_words,-1,_count * sizeof(word_t));
	}
	/**
	 * Sets all masks to <mask>
	 */
	void set_all(word_t mask) {
		for(size_t i = 0; i < _bits / BITS; ++i)
			set(i,mask);
	}
	/**
	 * Sets all bits to 0
	 */
	void clear_all() {
		memset(_words,0,_count * sizeof(word_t));
	}

private:
	MaskField(const MaskField &);
	MaskField& operator=(const MaskField &);

	size_t _bits;
	size_t _count;
	word_t *_words;
};

template<uint BITS>
static inline OStream &operator<<(OStream &os,const MaskField<BITS> &mf) {
	os << "Maskfield[";
	size_t count = mf._bits / BITS;
	for(size_t i = 0; i < count; ++i) {
		os.writef("%#0*x",Math::blockcount<uint>(BITS,4),mf.get(i));
		if(i < count - 1)
			os << ' ';
	}
	os << ']';
	return os;
}

}
