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

#include <utcb/Utcb.h>
#include <kobj/Ec.h>
#include <Format.h>
#include <assert.h>

namespace nul {

class UtcbFrameRef;

class TypedItem {
	friend class UtcbFrameRef;
public:
	enum {
		TYPE_XLT		= 0,
		TYPE_DEL		= 1,
	};

	TypedItem(Crd crd = Crd(0),unsigned aux = 0) : _crd(crd), _aux(aux) {
	}

private:
	Crd _crd;
	unsigned _aux;
};

class XltItem : public TypedItem {
public:
	XltItem(Crd crd)
		: TypedItem(crd,TYPE_XLT) {
	}
};

class DelItem : public TypedItem {
public:
	enum {
		FROM_HV	= 0x800,		// source = hypervisor
		UPD_GPT	= 0x400,		// update guest page table
		UPD_DPT	= 0x200,		// update DMA page table
	};

	DelItem(Crd crd,unsigned flags,unsigned hotspot = 0)
		: TypedItem(crd,TYPE_DEL | flags | (hotspot << 12)) {
	}
};

class UtcbFrameRef {
public:
	UtcbFrameRef() : _utcb(*Ec::current()->utcb()), _count(_utcb.untyped) {
	}
	virtual ~UtcbFrameRef() {
	}

	bool has_more() const {
		return _utcb.untyped > 0;
	}

	void reset() {
		_utcb.reset();
	}
	void set_receive_crd(Crd crd) {
		_utcb.crd = crd.value();
	}
	void add_typed(const TypedItem &item) {
		assert(_utcb.typed + _utcb.untyped + 2 <= Utcb::MAX_WORDS);
		_utcb.msg[Utcb::MAX_WORDS - ++_utcb.typed] = item._crd.value();
		_utcb.msg[Utcb::MAX_WORDS - ++_utcb.typed] = item._aux;
	}

	UtcbFrameRef &operator <<(unsigned value) {
		assert(_utcb.typed + _utcb.untyped + 1 <= Utcb::MAX_WORDS);
		_utcb.msg[_utcb.untyped++] = value;
		return *this;
	}
	UtcbFrameRef &operator <<(const TypedItem &item) {
		assert(_utcb.typed + _utcb.untyped + 2 <= Utcb::MAX_WORDS);
		_utcb.msg[_utcb.untyped++] = item._crd.value();
		_utcb.msg[_utcb.untyped++] = item._aux;
		return *this;
	}

	UtcbFrameRef &operator >>(unsigned &value) {
		assert(_utcb.untyped >= 1);
		value = _utcb.msg[_count - _utcb.untyped--];
		return *this;
	}
	UtcbFrameRef &operator >>(TypedItem &item) {
		assert(_utcb.untyped >= 2);
		item._aux = _utcb.msg[_count - _utcb.untyped--];
		item._crd = Crd(_utcb.msg[_count - _utcb.untyped--]);
		return *this;
	}

	void print(Format &fmt) const {
		_utcb.print(fmt);
	}

private:
	UtcbFrameRef(const UtcbFrameRef&);
	UtcbFrameRef& operator=(const UtcbFrameRef&);

protected:
	Utcb &_utcb;
	size_t _count;
};

class UtcbFrame : public UtcbFrameRef {
public:
	UtcbFrame() : UtcbFrameRef() {
		_utcb.push();
	}
	virtual ~UtcbFrame() {
		_utcb.pop();
	}
};

}
