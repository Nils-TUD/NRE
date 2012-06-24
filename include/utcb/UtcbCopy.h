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
#include <cstring>

namespace nul {

class UtcbFrameRef;
class UtcbFrame;

class Utcb : public UtcbBase {
	friend class UtcbFrameRef;
	friend class UtcbFrame;

	enum {
		POS_SLOT	= ((SIZE - sizeof(UtcbHead)) / sizeof(word_t)) / 2
	};

	static word_t *get_top(Utcb *frame) {
		size_t utcbtop = Math::round_dn<size_t>(reinterpret_cast<size_t>(frame + 1),Utcb::SIZE);
		return reinterpret_cast<word_t*>(utcbtop);
	}
	static word_t *get_top(Utcb *frame,size_t) {
		return get_top(frame);
	}
	static Utcb *get_current_frame(Utcb *base) {
		return base;
	}

	uint untyped_count() const {
		return msg[POS_SLOT] & 0xFFFF;
	}
	uint untyped_start() const {
		return POS_SLOT - untyped_count();
	}
	void add_untyped(int n) {
		short ut = untyped_count();
		msg[POS_SLOT] = (msg[POS_SLOT] & 0xFFFF0000) | (ut + n);
	}
	uint typed_count() const {
		return msg[POS_SLOT] >> 16;
	}
	uint typed_start() const {
		return POS_SLOT + 1 + typed_count();
	}
	void add_typed(int n) {
		short t = typed_count();
		msg[POS_SLOT] = (msg[POS_SLOT] & 0x0000FFFF) | (t + n) << 16;
	}

	Utcb *base() const {
		return reinterpret_cast<Utcb*>(reinterpret_cast<uintptr_t>(this) & ~(SIZE - 1));
	}
	size_t free_typed() const {
		return (WORDS - POS_SLOT - 1 - typed_count() - typed * 2) / 2;
	}
	size_t free_untyped() const {
		return POS_SLOT - untyped_count() - untyped - (sizeof(UtcbHead) / sizeof(word_t));
	}

	Utcb *push(word_t *&) {
		const int utcount = (sizeof(UtcbHead) / sizeof(word_t)) + untyped;
		const int tcount = typed * 2;
		word_t *utbackup = msg + untyped_start();
		word_t *tbackup = msg + typed_start();
		memcpy(utbackup - utcount,this,utcount * sizeof(word_t));
		utbackup[-(utcount + 1)] = utcount;
		memcpy(tbackup,get_top(this) - tcount,tcount * sizeof(word_t));
		tbackup[tcount] = tcount;
		add_untyped(utcount + 1);
		add_typed(tcount + 1);
		reset();
		return this;
	}
	void pop() {
		const word_t *utbackup = msg + untyped_start();
		const word_t *tbackup = msg + typed_start();
		const int utcount = utbackup[0];
		const int tcount = tbackup[-1];
		memcpy(this,utbackup + 1,utcount * sizeof(word_t));
		memcpy(get_top(this) - tcount,tbackup,tcount * sizeof(word_t));
		add_untyped(-(utcount + 1));
		add_typed(-(tcount + 1));
	}

public:
	void init() {
		msg[POS_SLOT] = 0;
	}
};

}
