/*
 * TODO comment me
 *
 * Copyright (C) 2012, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NUL.
 *
 * NUL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NUL is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 */

#pragma once

#include <utcb/UtcbHead.h>
#include <Syscalls.h>
#include <assert.h>

namespace nul {

class UtcbFrameRef;
class UtcbFrame;

class Utcb : public UtcbHead {
	friend class UtcbFrameRef;
	friend class UtcbFrame;

	typedef uint32_t word_t;
	enum {
		SIZE		= ExecEnv::PAGE_SIZE,
		WORDS		= SIZE / sizeof(word_t),
		MAX_TOP		= (SIZE / (4 * sizeof(word_t))) - 1,
		MAX_BOTTOM	= (SIZE / (2 * sizeof(word_t))) - 1,
	};

	word_t msg[(SIZE - sizeof(UtcbHead)) / sizeof(word_t)];

	// no construction and copying
	Utcb();
	~Utcb();
	Utcb(const Utcb&);
	Utcb& operator=(const Utcb&);

	Utcb *base() const {
		return reinterpret_cast<Utcb*>(reinterpret_cast<uintptr_t>(this) & ~(SIZE - 1));
	}
	size_t freewords() const {
		Utcb *utcb = base();
		size_t boff = utcb->bottom + sizeof(UtcbHead) / sizeof(word_t) + untyped;
		size_t toff = utcb->top + typed * 2;
		return WORDS - toff - boff;
	}

	Utcb *push(word_t *&toff) {
		Utcb *utcb = base();
		size_t off = (sizeof(UtcbHead) / sizeof(word_t)) + untyped;
		Utcb *frame = reinterpret_cast<Utcb*>(reinterpret_cast<word_t*>(utcb) + utcb->bottom + off);
		frame->top = utcb->top;
		frame->bottom = utcb->bottom;
		frame->reset();
		utcb->bottom += off;
		utcb->top += typed * 2;
		toff -= typed * 2;
		assert(utcb->bottom <= MAX_BOTTOM && utcb->top <= MAX_TOP);
		return frame;
	}
	void pop() {
		Utcb *utcb = base();
		utcb->bottom = this->bottom;
		utcb->top = this->top;
		assert(utcb->bottom <= MAX_BOTTOM && utcb->top <= MAX_TOP);
	}

public:
	void reset() {
		mtr = 0;
		crd = 0;
		crd_translate = 0;
	}

	void write(OStream &os) const;
};

}
