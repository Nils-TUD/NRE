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
#include <format/Format.h>
#include <cstring>
#include <assert.h>

namespace nul {

class Pt;
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
	friend class Pt;

public:
	UtcbFrameRef() : _utcb(Ec::current()->utcb()), _rpos() {
		_utcb = reinterpret_cast<Utcb*>(reinterpret_cast<Utcb::word_t*>(_utcb) + _utcb->bottom);
	}
	virtual ~UtcbFrameRef() {
	}

	void reset() {
		_utcb->reset();
		_rpos = 0;
	}

	size_t untyped() const {
		return _utcb->untyped;
	}
	size_t typed() const {
		return _utcb->typed;
	}
	bool has_more() const {
		return _rpos < _utcb->untyped;
	}

	void set_receive_crd(Crd crd) {
		_utcb->crd = crd.value();
	}
	void add_typed(const TypedItem &item) {
		assert(_utcb->freewords() >= 2);
		Utcb::word_t *top = _utcb->typed_begin();
		top[-(_utcb->typed * 2 + 1)] = item._aux;
		top[-(_utcb->typed * 2 + 2)] = item._crd.value();
		_utcb->typed++;
	}

	/*
	UtcbFrameRef &operator <<(const char *str,size_t len) {
		len = Util::blockcount(len,sizeof(Utcb::word_t));
		assert(_utcb->freewords() >= len + 1);
		_utcb->msg[_utcb->untyped++] = len;
		memcpy(_utcb->msg + _utcb->untyped,str,len);
		_utcb->untyped += len + 1;
		return *this;
	}
	*/
	template<typename T>
	UtcbFrameRef &operator <<(T value) {
		const size_t words = (sizeof(T) + sizeof(Utcb::word_t) - 1) / sizeof(Utcb::word_t);
		assert(_utcb->freewords() >= words);
		*reinterpret_cast<T*>(_utcb->msg + _utcb->untyped) = value;
		_utcb->untyped += words;
		return *this;
	}

	/*
	UtcbFrameRef &operator >>(char *str,size_t max) {
		assert(_rpos < _utcb->untyped);
		size_t len = _utcb->msg[_rpos++];
		memcpy(str,_utcb->msg + _rpos,Util::min(len,max));
		value = _utcb->msg[_rpos++];
		return *this;
	}
	*/
	template<typename T>
	UtcbFrameRef &operator >>(T &value) {
		const size_t words = (sizeof(T) + sizeof(Utcb::word_t) - 1) / sizeof(Utcb::word_t);
		assert(_rpos + words <= _utcb->untyped);
		value = *reinterpret_cast<T*>(_utcb->msg + _rpos);
		_rpos += words;
		return *this;
	}

	void print(Format &fmt) const {
		_utcb->print(fmt);
	}

private:
	UtcbFrameRef(const UtcbFrameRef&);
	UtcbFrameRef& operator=(const UtcbFrameRef&);

public:
	Utcb *_utcb;
	size_t _rpos;
};

class UtcbFrame : public UtcbFrameRef {
public:
	UtcbFrame() : UtcbFrameRef() {
		_utcb = _utcb->push();
	}
	virtual ~UtcbFrame() {
		_utcb->pop();
	}
};

class UtcbExcFrameRef : public UtcbFrameRef {
	class UtcbExc : public UtcbHead {
		struct Descriptor {
			uint16_t sel,ar;
			uint32_t limit,base,res;
			void set(uint16_t _sel,uint32_t _base,uint32_t _limit,uint16_t _ar) {
				sel = _sel;
				base = _base;
				limit = _limit;
				ar = _ar;
			}
		};

	public:
		uint32_t mtd;
		uint32_t inst_len,eip,efl;
		uint32_t intr_state,actv_state,inj_info,inj_error;
		union {
			struct {
				uint32_t eax,ecx,edx,ebx,esp,ebp,esi,edi;
			};
			uint32_t gpr[8];
		};
		uint64_t qual[2];
		uint32_t ctrl[2];
		int64_t tsc_off;
		uint32_t cr0,cr2,cr3,cr4;
		uint32_t dr7,sysenter_cs,sysenter_esp,sysenter_eip;
		Descriptor es,cs,ss,ds,fs,gs;
		Descriptor ld,tr,gd,id;
	};

public:
	UtcbExcFrameRef() : UtcbFrameRef() {
	}
	~UtcbExcFrameRef() {
	}

	UtcbExc *operator->() {
		return reinterpret_cast<UtcbExc*>(_utcb);
	}
};

}
