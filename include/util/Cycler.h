/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <util/LockPolicy.h>
#include <Assert.h>

namespace nre {

template<class It,class LockPolicy = LockPolicyNone>
class BaseCycler : public LockPolicy {
public:
	BaseCycler(It begin,It end) : LockPolicy(), _begin(begin), _end(end), _it(begin) {
	}

	void reset(It begin,It item,It end) {
		this->lock();
		_begin = begin;
		_it = item;
		_end = end;
		this->unlock();
	}

	bool valid() const {
		return _it != _end;
	}
	const It current() const {
		return _it;
	}

protected:
	It _begin;
	It _end;
	It _it;
};

template<class It,class LockPolicy = LockPolicyNone>
class ForwardCycler : public virtual BaseCycler<It,LockPolicy> {
public:
	ForwardCycler(It begin,It end) : BaseCycler<It,LockPolicy>(begin,end) {
	}

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

template<class It,class LockPolicy = LockPolicyNone>
class BackwardsCycler : public virtual BaseCycler<It,LockPolicy> {
public:
	BackwardsCycler(It begin,It end) : BaseCycler<It,LockPolicy>(begin,end) {
	}

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

template<class It,class LockPolicy = LockPolicyNone>
class Cycler : public ForwardCycler<It,LockPolicy>, public BackwardsCycler<It,LockPolicy> {
public:
	Cycler(It begin,It end) : BaseCycler<It,LockPolicy>(begin,end),
		ForwardCycler<It,LockPolicy>(begin,end),
		BackwardsCycler<It,LockPolicy>(begin,end) {
	}
};

}
