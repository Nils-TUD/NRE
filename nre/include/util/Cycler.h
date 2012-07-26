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

#include <util/LockPolicy.h>
#include <Assert.h>

namespace nre {

/**
 * Baseclass for all cyclers. A cycler allows to walk over an iterator (forward or backwards) and
 * if we're at the end or beginning, continue at the opposite side.
 * Note that you can specify a custom lock-policy to choose whether it should be thread-safe or not.
 */
template<class It,class LockPolicy = LockPolicyNone>
class BaseCycler : public LockPolicy {
public:
	/**
	 * Constructor
	 *
	 * @param begin the iterator-beginning
	 * @param end the iterator-end
	 */
	BaseCycler(It begin,It end) : LockPolicy(), _begin(begin), _end(end), _it(begin) {
	}

	/**
	 * Resets the cycler to given values. This may be used if an item has been removed from the
	 * iterator.
	 *
	 * @param begin the new beginning
	 * @param item the current item to set
	 * @param end the new end
	 */
	void reset(It begin,It item,It end) {
		this->lock();
		_begin = begin;
		_it = item;
		_end = end;
		this->unlock();
	}

	/**
	 * @return true if the current element is valid (always true if begin != end)
	 */
	bool valid() const {
		return _it != _end;
	}
	/**
	 * @return the current element
	 */
	const It current() const {
		return _it;
	}

protected:
	It _begin;
	It _end;
	It _it;
};

/**
 * A forward-cycler for iterators that can only move forward. Note that we use virtual inheritance
 * here, because Cycler inherits from two classes that share the same base-class. This way, we have
 * a lot of flexibility.
 */
template<class It,class LockPolicy = LockPolicyNone>
class ForwardCycler : public virtual BaseCycler<It,LockPolicy> {
public:
	/**
	 * Constructor
	 *
	 * @param begin the iterator-beginning
	 * @param end the iterator-end
	 */
	ForwardCycler(It begin,It end) : BaseCycler<It,LockPolicy>(begin,end) {
	}

	/**
	 * Moves the cycler to the next item and returns it. That is, if we're not at the end yet,
	 * the iterator is simply moved forward. Otherwise we continue at the beginning.
	 *
	 * @return the next item
	 */
	It next() {
		this->lock();
		if(this->_it != this->_end)
			++this->_it;
		if(this->_it == this->_end)
			this->_it = this->_begin;
		this->unlock();
		return this->_it;
	}
};

/**
 * A backwards-cycler for iterators that can only move backwards.
 */
template<class It,class LockPolicy = LockPolicyNone>
class BackwardsCycler : public virtual BaseCycler<It,LockPolicy> {
public:
	/**
	 * Constructor
	 *
	 * @param begin the iterator-beginning
	 * @param end the iterator-end
	 */
	BackwardsCycler(It begin,It end) : BaseCycler<It,LockPolicy>(begin,end) {
	}

	/**
	 * Moves the cycler to the previous item and returns it. That is, if we're not at the beginning
	 * yet, the iterator is simply moved backwards. Otherwise we continue at the end.
	 *
	 * @return the next item
	 */
	It prev() {
		this->lock();
		if(this->_it == this->_begin)
			this->_it = this->_end;
		if(this->_it != this->_begin)
			--this->_it;
		this->unlock();
		return this->_it;
	}
};

/**
 * A cycler for iterators that can move forward and backwards.
 */
template<class It,class LockPolicy = LockPolicyNone>
class Cycler : public ForwardCycler<It,LockPolicy>, public BackwardsCycler<It,LockPolicy> {
public:
	/**
	 * Constructor
	 *
	 * @param begin the iterator-beginning
	 * @param end the iterator-end
	 */
	Cycler(It begin,It end) : BaseCycler<It,LockPolicy>(begin,end),
		ForwardCycler<It,LockPolicy>(begin,end),
		BackwardsCycler<It,LockPolicy>(begin,end) {
	}
};

}
