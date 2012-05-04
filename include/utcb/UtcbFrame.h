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
#include <cap/CapRange.h>
#include <stream/OStream.h>
#include <kobj/Ec.h>
#include <String.h>
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

	Crd crd() const {
		return _crd;
	}
	unsigned aux() const {
		return _aux;
	}

private:
	Crd _crd;
	unsigned _aux;
};

class XltItem : public TypedItem {
public:
	XltItem(Crd crd) : TypedItem(crd,TYPE_XLT) {
	}
};

class DelItem : public TypedItem {
public:
	enum {
		FROM_HV	= 0x800,		// source = hypervisor
		UPD_GPT	= 0x400,		// update guest page table
		UPD_DPT	= 0x200,		// update DMA page table
	};

	DelItem(Crd crd,unsigned flags,unsigned hotspot = 0) : TypedItem(crd,TYPE_DEL | flags | (hotspot << 12)) {
	}
};

class UtcbFrameRef {
	friend class Pt;
	friend class Utcb;

	static Utcb::word_t *get_top(Utcb *frame,size_t toff) {
		size_t utcbtop = Util::rounddown<size_t>(reinterpret_cast<size_t>(frame + 1),Utcb::SIZE);
		return reinterpret_cast<Utcb::word_t*>(utcbtop) - toff;
	}
	static Utcb *get_frame(Utcb *base,size_t off) {
		return reinterpret_cast<Utcb*>(reinterpret_cast<Utcb::word_t*>(base) + off);
	}

	void check_write(size_t words) {
		if(_utcb->freewords() < words)
			throw Exception("Utcb has not enough capacity");
	}
	void check_untyped_read(size_t words) {
		if(_upos + words > untyped())
			throw Exception("Utcb has no more untyped items");
	}
	void check_typed_read(size_t words) {
		if(_tpos + words > typed())
			throw Exception("Utcb has no more typed items");
	}

	UtcbFrameRef(Utcb *utcb,size_t top) : _utcb(utcb), _top(get_top(utcb,top)), _upos(), _tpos() {
	}
public:
	UtcbFrameRef(Utcb *utcb = Ec::current()->utcb()) : _utcb(utcb), _top(get_top(_utcb,_utcb->top)), _upos(), _tpos() {
		_utcb = get_frame(_utcb,_utcb->bottom);
	}
	virtual ~UtcbFrameRef() {
	}

	void clear() {
		_utcb->mtr = 0;
	}
	void reset() {
		_utcb->reset();
		_upos = _tpos = 0;
	}

	size_t untyped() const {
		return _utcb->untyped;
	}
	size_t typed() const {
		return _utcb->typed;
	}
	bool has_more_typed() const {
		return _tpos < typed();
	}
	bool has_more_untyped() const {
		return _upos < untyped();
	}

	Crd get_receive_crd() const {
		return Crd(_utcb->crd);
	}
	void set_receive_crd(Crd crd) {
		_utcb->crd = crd.value();
	}

	void delegate(cap_t cap,uint flags = 0) {
		add_typed(DelItem(Crd(cap,0,DESC_CAP_ALL),flags,0));
	}
	void delegate(const CapRange& range,uint flags = 0) {
		uintptr_t hotspot = range.hotspot() ? range.hotspot() : range.start();
		size_t count = range.count();
		uintptr_t start = range.start();
		while(count > 0) {
			uint minshift = Util::minshift(start,count);
			add_typed(DelItem(Crd(start,minshift,range.attr()),flags,hotspot));
			start += 1 << minshift;
			hotspot += 1 << minshift;
			count -= 1 << minshift;
		}
	}
	void add_typed(const TypedItem &item) {
		// ensure that we're the current frame
		assert(get_frame(_utcb->base(),_utcb->base()->bottom) == _utcb);
		check_write(2);
		_top[-(_utcb->typed * 2 + 1)] = item._aux;
		_top[-(_utcb->typed * 2 + 2)] = item._crd.value();
		_utcb->typed++;
	}
	void get_typed(TypedItem& item) {
		check_typed_read(1);
		item._aux = _top[-(_tpos * 2 + 1)];
		item._crd = Crd(_top[-(_tpos * 2 + 2)]);
		_tpos++;
	}

	template<typename T>
	UtcbFrameRef &operator <<(const T& value) {
		const size_t words = (sizeof(T) + sizeof(Utcb::word_t) - 1) / sizeof(Utcb::word_t);
		check_write(words);
		assert(get_frame(_utcb->base(),_utcb->base()->bottom) == _utcb);
		*reinterpret_cast<T*>(_utcb->msg + untyped()) = value;
		_utcb->untyped += words;
		return *this;
	}
	UtcbFrameRef &operator <<(const String& value) {
		const size_t words = Util::blockcount(value.length(),sizeof(Utcb::word_t)) + 1;
		check_write(words);
		assert(get_frame(_utcb->base(),_utcb->base()->bottom) == _utcb);
		*reinterpret_cast<size_t*>(_utcb->msg + untyped()) = value.length();
		memcpy(_utcb->msg + untyped() + 1,value.str(),value.length());
		_utcb->untyped += words;
		return *this;
	}

	template<typename T>
	UtcbFrameRef &operator >>(T &value) {
		const size_t words = (sizeof(T) + sizeof(Utcb::word_t) - 1) / sizeof(Utcb::word_t);
		check_untyped_read(words);
		value = *reinterpret_cast<T*>(_utcb->msg + _upos);
		_upos += words;
		return *this;
	}
	UtcbFrameRef &operator >>(String &value) {
		check_untyped_read(1);
		size_t len = *reinterpret_cast<size_t*>(_utcb->msg + _upos);
		const size_t words = Util::blockcount(len,sizeof(Utcb::word_t)) + 1;
		check_untyped_read(words);
		value.reset(reinterpret_cast<const char*>(_utcb->msg + _upos + 1),len);
		_upos += words;
		return *this;
	}

	void write(OStream &os) const {
		os.writef("UtcbFrame @ %p:\n",_utcb);
		os.writef("\tCrd: %u\n",_utcb->crd);
		os.writef("\tCrd translate: %u\n",_utcb->crd_translate);
		os.writef("\tUntyped: %u\n",untyped());
		for(size_t i = 0; i < untyped(); ++i)
			os.writef("\t\t%zu: %#x\n",i,_utcb->msg[i]);
		os.writef("\tTyped: %u\n",typed());
		for(size_t i = 0; i < typed() * 2; i += 2)
			os.writef("\t\t%zu: %#x : %#x\n",i,_top[-(i + 1)],_top[-(i + 2)]);
	}

private:
	UtcbFrameRef(const UtcbFrameRef&);
	UtcbFrameRef& operator=(const UtcbFrameRef&);

protected:
	Utcb *_utcb;
	Utcb::word_t *_top;
	size_t _upos;
	size_t _tpos;
};

class UtcbFrame : public UtcbFrameRef {
public:
	UtcbFrame() : UtcbFrameRef() {
		_utcb = _utcb->push(_top);
	}
	virtual ~UtcbFrame() {
		_utcb->pop();
	}

	// TODO cloning?
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
