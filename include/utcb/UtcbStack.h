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
#include <util/Math.h>

#if USE_UTCB_STACKING

namespace nul {

class UtcbFrameRef;
class UtcbFrame;
class OStream;
OStream &operator<<(OStream &os,const Utcb &utcb);

/**
 * The stack-based UTCB implementation. Allows it to push and pop frames on the UTCB by simply
 * adjusting the offsets. That is, by using a bit of arithmethic instead of copying data.
 */
class Utcb : public UtcbBase {
	friend class UtcbFrameRef;
	friend class UtcbFrame;
	friend 	OStream &operator<<(OStream &os,const UtcbBase &utcb);

	enum {
		MAX_TOP		= (SIZE / (4 * sizeof(word_t))) - 1,
		MAX_BOTTOM	= (SIZE / (2 * sizeof(word_t))) - 1,
	};

	/**
	 * @param frame the current frame
	 * @return the pointer to the start of the typed items
	 */
	static word_t *get_top(Utcb *frame) {
		return get_top(frame,frame->top);
	}
	/**
	 * @param frame the current frame
	 * @param toff the offset from top (in words)
	 * @return the pointer to the start of the typed items
	 */
	static word_t *get_top(Utcb *frame,size_t toff) {
		size_t utcbtop = Math::round_dn<size_t>(reinterpret_cast<size_t>(frame + 1),Utcb::SIZE);
		return reinterpret_cast<word_t*>(utcbtop) - toff;
	}
	/**
	 * @param base the UTCB base
	 * @return the current frame
	 */
	static Utcb *get_current_frame(Utcb *base) {
		return reinterpret_cast<Utcb*>(reinterpret_cast<word_t*>(base) + base->bottom);
	}

	/**
	 * @return the pointer to the UTCB base, i.e. the first frame
	 */
	Utcb *base() const {
		return reinterpret_cast<Utcb*>(reinterpret_cast<uintptr_t>(this) & ~(SIZE - 1));
	}
	/**
	 * @return the number of free typed items (i.e. the number of typed items you can still add)
	 */
	size_t free_typed() const {
		Utcb *utcb = base();
		return (WORDS - (utcb->bottom + sizeof(UtcbHead) / sizeof(word_t) + untyped)) / 2;
	}
	/**
	 * @return the number of free untyped items
	 */
	size_t free_untyped() const {
		Utcb *utcb = base();
		return WORDS - (utcb->top + typed * 2);
	}

	// only used for UtcbCopy
	void push_layer() {
	}
	void pop_layer() {
	}

	/**
	 * Creates a new frame in the UTCB.
	 *
	 * @param toff will be set to the current offset from the top
	 * @return the pointer to the created frame
	 */
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
	/**
	 * Removes the topmost frame from the UTCB
	 */
	void pop() {
		Utcb *utcb = base();
		utcb->bottom = this->bottom;
		utcb->top = this->top;
		assert(utcb->bottom <= MAX_BOTTOM && utcb->top <= MAX_TOP);
	}
};

}

#endif
