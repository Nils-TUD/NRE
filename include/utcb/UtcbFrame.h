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

#include <arch/UtcbExcLayout.h>
#include <utcb/Utcb.h>
#include <cap/CapRange.h>
#include <cap/CapSpace.h>
#include <Exception.h>
#include <kobj/Thread.h>
#include <String.h>
#include <cstring>
#include <Assert.h>
#include <util/Math.h>

namespace nre {

class Pt;
class UtcbFrameRef;
class UtcbExcFrameRef;
OStream &operator<<(OStream &os,const Utcb &utcb);
OStream &operator<<(OStream &os,const UtcbFrameRef &frm);
OStream &operator<<(OStream &os,const UtcbExc::Descriptor &desc);
OStream &operator<<(OStream &os,const UtcbExcFrameRef &frm);

/**
 * The exception that will be thrown in case of UTCB errors
 */
class UtcbException : public Exception {
public:
	explicit UtcbException(ErrorCode code) throw() : Exception(code) {
	}
};

/**
 * There are basically two classes for working with the UTCB. These are UtcbFrame and UtcbFrameRef.
 * The first one creates a new frame in the UTCB. Thus, this should be used when you want to put
 * something in the UTCB and call a portal. The second one gives you access to the current frame.
 * Therefore, you can use it in portals to retrieve the items the caller has sent you.
 * Besides that, there is no difference between these classes. With both you can add items, retrieve
 * items, translate or delegate caps and receive caps.
 * To work with them, just put an instance of one of the classes on the stack (or on the heap if you
 * like, but that not really make sense ;)). The constructor and destructor will perform the
 * corresponding action.
 */

/**
 * The class UtcbFrameRef gives you access to the current frame in the UTCB.
 */
class UtcbFrameRef {
	friend class Pt;
	friend class Utcb;
	friend OStream &operator<<(OStream &os,const Utcb &utcb);
	friend 	OStream &operator<<(OStream &os,const UtcbFrameRef &frm);
	friend OStream &operator<<(OStream &os,const UtcbExcFrameRef &frm);

public:
	enum DelFlags {
		NONE	= 0,
		FROM_HV	= 0x800,		// source = hypervisor
		UPD_GPT	= 0x400,		// update guest page table
		UPD_DPT	= 0x200,		// update DMA page table
	};

private:
	/**
	 * Base class for all kinds of typed items
	 */
	class TypedItem {
		friend class UtcbFrameRef;
	public:
		enum {
			TYPE_XLT		= 0,
			TYPE_DEL		= 1,
		};

		explicit TypedItem(Crd crd = Crd(0),word_t aux = 0) : _crd(crd), _aux(aux) {
		}

		Crd crd() const {
			return _crd;
		}
		word_t aux() const {
			return _aux;
		}

	private:
		Crd _crd;
		word_t _aux;
	};

	/**
	 * For the translation of capabilities
	 */
	class XltItem : public TypedItem {
	public:
		explicit XltItem(Crd crd) : TypedItem(crd,TYPE_XLT) {
		}
	};

	/**
	 * For the delegation of capabilities
	 */
	class DelItem : public TypedItem {
	public:
		explicit DelItem(Crd crd,unsigned flags,word_t hotspot = 0)
			: TypedItem(crd,TYPE_DEL | flags | (hotspot << 12)) {
		}
	};

	void check_untyped_write(size_t u) {
		if(free_untyped() < u)
			throw UtcbException(E_CAPACITY);
	}
	void check_typed_write(size_t t) {
		if(free_typed() < t)
			throw UtcbException(E_CAPACITY);
	}
	void check_untyped_read(size_t u) {
		if(_upos + u > untyped())
			throw UtcbException(E_UTCB_UNTYPED);
	}
	void check_typed_read(size_t t) {
		if(_tpos + t > typed())
			throw UtcbException(E_UTCB_TYPED);
	}

	explicit UtcbFrameRef(Utcb *utcb,size_t top)
			: _utcb(utcb), _top(Utcb::get_top(utcb,top)), _upos(), _tpos() {
		_utcb->push_layer();
	}
public:
	/**
	 * Gives access to the given UTCB
	 *
	 * @param utcb the UTCB to access (the one of the current thread by default)
	 */
	explicit UtcbFrameRef(Utcb *utcb = Thread::current()->utcb())
			: _utcb(utcb), _top(Utcb::get_top(_utcb)), _upos(), _tpos() {
		_utcb = Utcb::get_current_frame(_utcb);
		_utcb->push_layer();
	}
	virtual ~UtcbFrameRef() {
		_utcb->pop_layer();
	}

	/**
	 * Clears the UTCB in the sense that no untyped or typed items are present anymore
	 */
	void clear() {
		_utcb->mtr = 0;
	}
	/**
	 * Resets the UTCB in the sense that the complete header is set to its default state.
	 * Additionally, the read position for untyped and typed items is reset
	 */
	void reset() {
		_utcb->reset();
		_upos = _tpos = 0;
	}

	/**
	 * @return the currently set receive window for capability delegations
	 */
	Crd delegation_window() const {
		return Crd(_utcb->crd);
	}
	/**
	 * Sets the receive window for capability delegations to <crd>
	 */
	void delegation_window(Crd crd) {
		_utcb->crd = crd.value();
	}
	/**
	 * @return the currently set receive window for capability translations
	 */
	Crd translation_window() const {
		return Crd(_utcb->crd_translate);
	}
	/**
	 * Sets the receive window for capability translations to <crd>
	 */
	void translation_window(Crd crd) {
		_utcb->crd_translate = crd.value();
	}

	/**
	 * Prepares the UTCB frame for capability translations
	 *
	 * @param offset the allowed offset of translations (default 0)
	 * @param order the allowed order of translations (default 31)
	 * @param attr the allowed attributes (default Crd::OBJ_ALL)
	 */
	void accept_translates(word_t offset = 0,uint order = 31,uint attr = Crd::OBJ_ALL) {
		translation_window(Crd(offset,order,attr));
	}
	/**
	 * Prepares the UTCB frame again(!) for capability delegations. That is, it reserves new cap
	 * selectors in your CapSpace.
	 */
	void accept_delegates() {
		accept_delegates(delegation_window().order());
	}
	/**
	 * Prepares the UTCB frame for capability delegations. That is, it allocates new cap selectors
	 * in your CapSpace.
	 *
	 * @param order set the receive window size to 2^<order>
	 * @param attr the allowed attributes (default Crd::OBJ_ALL)
	 */
	void accept_delegates(uint order,uint attr = Crd::OBJ_ALL) {
		capsel_t caps = CapSpace::get().allocate(1 << order,1 << order);
		delegation_window(Crd(caps,order,attr));
	}

	/**
	 * Finishes the input phase when receiving data from the UTCB. It ensures that no further
	 * capabilities have been translated or delegated to you and no more data has been sent.
	 * Additionally, it does a clear() to let to start with putting in the response.
	 *
	 * @throws UtcbException if there is more data in the UTCB
	 */
	void finish_input() {
		if(has_more_untyped() || has_more_typed())
			throw UtcbException(E_ARGS_INVALID);
		clear();
	}

	/**
	 * @return the number of still free untyped items
	 */
	size_t free_untyped() const {
		return _utcb->free_untyped();
	}
	/**
	 * @return the number of still free typed items
	 */
	size_t free_typed() const {
		return _utcb->free_typed();
	}
	/**
	 * @return the number of existing untyped items
	 */
	size_t untyped() const {
		return _utcb->untyped;
	}
	/**
	 * @return the number of existing typed items
	 */
	size_t typed() const {
		return _utcb->typed;
	}
	/**
	 * @return true if there are more typed items to read (see get_typed())
	 */
	bool has_more_typed() const {
		return _tpos < typed();
	}
	/**
	 * @return true if there are more untyped items to read (see the shift operators)
	 */
	bool has_more_untyped() const {
		return _upos < untyped();
	}

	/**
	 * Puts in a typed item to translate <cap>
	 *
	 * @param cap the capability selector
	 * @param attr the attributes to translate
	 * @throws UtcbException if there is not enough space
	 */
	void translate(capsel_t cap,uint attr = Crd::OBJ_ALL) {
		add_typed(XltItem(Crd(cap,0,attr)));
	}
	/**
	 * Puts in as many typed items as required to translate the given CapRange. Note that the
	 * space in the UTCB might not be sufficient!
	 *
	 * @param range the CapRange to translate
	 * @throws UtcbException if there is not enough space
	 */
	void translate(const CapRange& range) {
		size_t count = range.count();
		uintptr_t start = range.start();
		while(count > 0) {
			uint minshift = Math::minshift(start,count);
			add_typed(XltItem(Crd(start,minshift,range.attr())));
			start += 1 << minshift;
			count -= 1 << minshift;
		}
	}

	/**
	 * Puts in a typed item to delegate <cap>
	 *
	 * @param cap the capability selector
	 * @param hotspot the hotspot to delegate it to (default 0)
	 * @param flags the flags for the delegation (default NONE)
	 * @param attr the attributes to delegate (default Crd::OBJ_ALL)
	 */
	void delegate(capsel_t cap,uintptr_t hotspot = CapRange::NO_HOTSPOT,DelFlags flags = NONE,
			uint attr = Crd::OBJ_ALL) {
		add_typed(DelItem(Crd(cap,0,attr),flags,hotspot != CapRange::NO_HOTSPOT ? hotspot : cap));
	}
	/**
	 * Puts in as many typed items as required to delegate the given CapRange. Note that the
	 * space in the UTCB might not be sufficient!
	 *
	 * @param range the CapRange to delegate
	 * @param flags the flags for the delegation (default NONE)
	 */
	void delegate(const CapRange& range,DelFlags flags = NONE) {
		uintptr_t hotspot = range.hotspot() != CapRange::NO_HOTSPOT ? range.hotspot() : range.start();
		size_t count = range.count();
		uintptr_t start = range.start();
		while(count > 0) {
			uint minshift = Math::minshift(start | hotspot,count);
			add_typed(DelItem(Crd(start,minshift,range.attr()),flags,hotspot));
			start += 1 << minshift;
			hotspot += 1 << minshift;
			count -= 1 << minshift;
		}
	}

	/**
	 * Retrieves the next typed item from the UTCB frame. It checks whether the item exists,
	 * is a translation and that the order is <order>.
	 *
	 * @param order the order of the Crd you expect
	 * @return the translated Crd
	 * @throws UtcbException if something is not right
	 */
	Crd get_translated(uint order = 0) {
		TypedItem ti;
		get_typed(ti);
		if(ti.crd().is_null() || (ti.aux() & TypedItem::TYPE_DEL) || ti.crd().order() != order)
			throw UtcbException(E_ARGS_INVALID);
		return ti.crd();
	}
	/**
	 * Retrieves the next typed item from the UTCB frame. It checks whether the item exists,
	 * is a delegation and that the order is <order>.
	 *
	 * @param order the order of the Crd you expect
	 * @return the delegated Crd
	 * @throws UtcbException if something is not right
	 */
	Crd get_delegated(uint order = 0) {
		TypedItem ti;
		get_typed(ti);
		if(ti.crd().is_null() || !(ti.aux() & TypedItem::TYPE_DEL) || ti.crd().order() != order)
			throw UtcbException(E_ARGS_INVALID);
		return ti.crd();
	}
	/**
	 * Retrieves the next typed item from the UTCB frame into <item>
	 *
	 * @param item the place to write it to
	 */
	void get_typed(TypedItem& item) {
		check_typed_read(1);
		item._aux = _top[-(_tpos * 2 + 1)];
		item._crd = Crd(_top[-(_tpos * 2 + 2)]);
		_tpos++;
	}

	/**
	 * Writes the given object as untyped item into the UTCB frame. Note that there might not be
	 * enough space left in the UTCB.
	 *
	 * @param value the object to write
	 * @return *this
	 * @throws UtcbException if there is not enough space
	 */
	template<typename T>
	UtcbFrameRef &operator <<(const T& value) {
		const size_t words = (sizeof(T) + sizeof(word_t) - 1) / sizeof(word_t);
		check_untyped_write(words);
		assert(Utcb::get_current_frame(_utcb->base()) == _utcb);
		*reinterpret_cast<T*>(_utcb->msg + untyped()) = value;
		_utcb->untyped += words;
		return *this;
	}
	UtcbFrameRef &operator <<(const String& value) {
		const size_t words = Math::blockcount(value.length(),sizeof(word_t)) + 1;
		check_untyped_write(words);
		assert(Utcb::get_current_frame(_utcb->base()) == _utcb);
		*reinterpret_cast<size_t*>(_utcb->msg + untyped()) = value.length();
		memcpy(_utcb->msg + untyped() + 1,value.str(),value.length());
		_utcb->untyped += words;
		return *this;
	}

	/**
	 * Reads the next untyped item from the UTCB frame. Of course, you need to know what object you
	 * receive.
	 *
	 * @param value the place to write to
	 * @return *this
	 * @throws UtcbException if there is no untyped item anymore
	 */
	template<typename T>
	UtcbFrameRef &operator >>(T &value) {
		const size_t words = (sizeof(T) + sizeof(word_t) - 1) / sizeof(word_t);
		check_untyped_read(words);
		value = *reinterpret_cast<T*>(_utcb->msg + _upos);
		_upos += words;
		return *this;
	}
	UtcbFrameRef &operator >>(String &value) {
		check_untyped_read(1);
		size_t len = *reinterpret_cast<size_t*>(_utcb->msg + _upos);
		const size_t words = Math::blockcount(len,sizeof(word_t)) + 1;
		check_untyped_read(words);
		value.reset(reinterpret_cast<const char*>(_utcb->msg + _upos + 1),len);
		_upos += words;
		return *this;
	}

private:
	void add_typed(const TypedItem &item) {
		// ensure that we're the current frame
		assert(Utcb::get_current_frame(_utcb->base()) == _utcb);
		check_typed_write(1);
		_top[-(_utcb->typed * 2 + 1)] = item._aux;
		_top[-(_utcb->typed * 2 + 2)] = item._crd.value();
		_utcb->typed++;
	}

	UtcbFrameRef(const UtcbFrameRef&);
	UtcbFrameRef& operator=(const UtcbFrameRef&);

protected:
	Utcb *_utcb;
	word_t *_top;
	size_t _upos;
	size_t _tpos;
};

/**
 * The UtcbFrame creates a new frame in the UTCB.
 */
class UtcbFrame : public UtcbFrameRef {
public:
	/**
	 * The constructor creates a new frame in the UTCB
	 */
	explicit UtcbFrame() : UtcbFrameRef() {
		_utcb = _utcb->push(_top);
	}
	/**
	 * The destructor removes the topmost frame from the UTCB
	 */
	virtual ~UtcbFrame() {
		_utcb->pop();
	}
};

/**
 * This class can be used to access the exception state NOVA passed to you. You can access it
 * by using the overloaded operator->.
 */
class UtcbExcFrameRef : public UtcbFrameRef {
public:
	explicit UtcbExcFrameRef() : UtcbFrameRef() {
	}

	/**
	 * @return the exception state
	 */
	UtcbExc *operator->() {
		return reinterpret_cast<UtcbExc*>(_utcb);
	}
	const UtcbExc *operator->() const {
		return reinterpret_cast<const UtcbExc*>(_utcb);
	}
};

}
