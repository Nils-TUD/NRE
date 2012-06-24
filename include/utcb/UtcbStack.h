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

#include <utcb/UtcbBase.h>
#include <Assert.h>
#include <Math.h>

#if USE_UTCB_STACKING

namespace nul {

class UtcbFrameRef;
class UtcbFrame;
class OStream;
OStream &operator<<(OStream &os,const Utcb &utcb);

class Utcb : public UtcbBase {
	friend class UtcbFrameRef;
	friend class UtcbFrame;
	friend 	OStream &operator<<(OStream &os,const UtcbBase &utcb);

	enum {
		MAX_TOP		= (SIZE / (4 * sizeof(word_t))) - 1,
		MAX_BOTTOM	= (SIZE / (2 * sizeof(word_t))) - 1,
	};

	static word_t *get_top(Utcb *frame) {
		return get_top(frame,frame->top);
	}
	static word_t *get_top(Utcb *frame,size_t toff) {
		size_t utcbtop = Math::round_dn<size_t>(reinterpret_cast<size_t>(frame + 1),Utcb::SIZE);
		return reinterpret_cast<word_t*>(utcbtop) - toff;
	}
	static Utcb *get_current_frame(Utcb *base) {
		return reinterpret_cast<Utcb*>(reinterpret_cast<word_t*>(base) + base->bottom);
	}

	Utcb *base() const {
		return reinterpret_cast<Utcb*>(reinterpret_cast<uintptr_t>(this) & ~(SIZE - 1));
	}
	size_t free_typed() const {
		Utcb *utcb = base();
		return WORDS - (utcb->bottom + sizeof(UtcbHead) / sizeof(word_t) + untyped);
	}
	size_t free_untyped() const {
		Utcb *utcb = base();
		return (WORDS - (utcb->top + typed * 2)) / 2;
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
};

}

#endif
