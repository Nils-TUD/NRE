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

#include <kobj/LocalThread.h>
#include <kobj/Pt.h>
#include <kobj/Sm.h>
#include <kobj/UserSm.h>
#include <ipc/ServiceInstance.h>
#include <mem/DataSpace.h>
#include <utcb/UtcbFrame.h>
#include <Exception.h>
#include <util/ScopedPtr.h>
#include <util/BitField.h>
#include <RCU.h>
#include <CPU.h>
#include <util/Math.h>

namespace nre {

class Service;
template<class T>
class SessionIterator;

class ServiceException : public Exception {
public:
	explicit ServiceException(ErrorCode code) throw() : Exception(code) {
	}
};

class SessionData : public RCUObject {
	friend class Service;
	friend class ServiceInstance;

public:
	explicit SessionData(Service *s,size_t id,capsel_t pts,Pt::portal_func func);
	virtual ~SessionData() {
		for(uint i = 0; i < CPU::count(); ++i)
			delete _objs[i];
		delete[] _objs;
	}

	size_t id() const {
		return _id;
	}
	capsel_t caps() const {
		return _caps;
	}

protected:
	virtual void invalidate() {
	}

	virtual void accept_ds(DataSpace *) {
		throw ServiceException(E_ARGS_INVALID);
	}

private:
	size_t _id;
	capsel_t _caps;
	ObjCap **_objs;
};

class Service {
	friend class ServiceInstance;
	template<class T>
	friend class SessionIterator;

public:
	enum Command {
		OPEN_SESSION,
		SHARE_DATASPACE,
		CLOSE_SESSION
	};

	enum {
		MAX_SESSIONS		= 32
	};

	explicit Service(const char *name,Pt::portal_func portal = 0)
		: _regcaps(CapSpace::get().allocate(CPU::count(),CPU::count())),
		  _caps(CapSpace::get().allocate(MAX_SESSIONS * CPU::count(),MAX_SESSIONS * CPU::count())),
		  _sm(), _name(name), _func(portal), _insts(new ServiceInstance*[CPU::count()]),
		  _reg_cpus(), _sessions() {
		for(size_t i = 0; i < CPU::count(); ++i)
			_insts[i] = 0;
	}
	virtual ~Service() {
		for(size_t i = 0; i < MAX_SESSIONS; ++i)
			delete _sessions[i];
		for(size_t i = 0; i < CPU::count(); ++i)
			delete _insts[i];
		delete[] _insts;
		CapSpace::get().free(_caps,MAX_SESSIONS * CPU::count());
		CapSpace::get().free(_regcaps,CPU::count());
	}

	const char *name() const {
		return _name;
	}
	Pt::portal_func portal() const {
		return _func;
	}
	capsel_t caps() const {
		return _caps;
	}
	const BitField<Hip::MAX_CPUS> &available() const {
		return _reg_cpus;
	}

	template<class T>
	SessionIterator<T> sessions_begin();
	template<class T>
	SessionIterator<T> sessions_end();

	template<class T>
	T *get_session(capsel_t pid) {
		return get_session_by_id<T>((pid - _caps) / CPU::count());
	}
	template<class T>
	T *get_session_by_id(size_t id) {
		T *sess = static_cast<T*>(rcu_dereference(_sessions[id]));
		if(!sess)
			throw ServiceException(E_ARGS_INVALID);
		return sess;
	}

	void provide_on(cpu_t cpu) {
		assert(_insts[cpu] == 0);
		_insts[cpu] = new ServiceInstance(this,_regcaps + cpu,cpu);
		_reg_cpus.set(cpu);
	}
	LocalThread *get_ec(cpu_t cpu) const {
		return _insts[cpu] != 0 ? &_insts[cpu]->ec() : 0;
	}
	void reg() {
		UtcbFrame uf;
		uf.delegate(CapRange(_regcaps,Math::next_pow2<size_t>(CPU::count()),Crd::OBJ_ALL));
		uf << String(_name) << _reg_cpus;
		CPU::current().reg_pt->call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw ServiceException(res);
	}
	void unreg() {
		UtcbFrame uf;
		uf << String(_name);
		CPU::current().unreg_pt->call(uf);
		ErrorCode res;
		uf >> res;
		if(res != E_SUCCESS)
			throw ServiceException(res);
	}

	// TODO wrong place?
	void wait() {
		Sm sm(0);
		sm.down();
	}

	UserSm &sm() {
		return _sm;
	}

protected:
	void add_session(SessionData *sess) {
		rcu_assign_pointer(_sessions[sess->id()],sess);
		created_session(sess->id());
	}
	void remove_session(SessionData *sess) {
		rcu_assign_pointer(_sessions[sess->id()],0);
		sess->invalidate();
		RCU::invalidate(sess);
		RCU::gc(true);
	}

private:
	virtual SessionData *create_session(size_t id,capsel_t pts,Pt::portal_func func) {
		return new SessionData(this,id,pts,func);
	}
	virtual void created_session(size_t) {
	}

	SessionData *new_session() {
		ScopedLock<UserSm> guard(&_sm);
		for(size_t i = 0; i < MAX_SESSIONS; ++i) {
			if(_sessions[i] == 0) {
				add_session(create_session(i,_caps + i * CPU::count(),_func));
				return _sessions[i];
			}
		}
		throw ServiceException(E_CAPACITY);
	}

	void destroy_session(capsel_t pid) {
		ScopedLock<UserSm> guard(&_sm);
		size_t i = (pid - _caps) / CPU::count();
		SessionData *sess = _sessions[i];
		if(!sess)
			throw ServiceException(E_NOT_FOUND);
		remove_session(sess);
	}

	Service(const Service&);
	Service& operator=(const Service&);

	capsel_t _regcaps;
	capsel_t _caps;
	UserSm _sm;
	const char *_name;
	Pt::portal_func _func;
	ServiceInstance **_insts;
	BitField<Hip::MAX_CPUS> _reg_cpus;
	SessionData *_sessions[MAX_SESSIONS];
};

template<class T>
class SessionIterator {
	friend class Service;

public:
	explicit SessionIterator(Service *s,ssize_t pos = 0) : _s(s), _pos(pos), _last(next()) {
	}
	~SessionIterator() {
	}

	T& operator *() const {
		return *_last;
	}
	T *operator ->() const {
		return &operator *();
	}
	SessionIterator& operator ++() {
		if(_pos < Service::MAX_SESSIONS - 1) {
			_pos++;
			_last = next();
		}
		return *this;
	}
	SessionIterator operator ++(int) {
		SessionIterator<T> tmp(*this);
		operator++();
		return tmp;
	}
	SessionIterator& operator --() {
		if(_pos > 0) {
			_pos--;
			_last = prev();
		}
		return *this;
	}
	SessionIterator operator --(int) {
		SessionIterator<T> tmp(*this);
		operator++();
		return tmp;
	}
	bool operator ==(const SessionIterator<T>& rhs) const {
		return _pos == rhs._pos;
	}
	bool operator !=(const SessionIterator<T>& rhs) const {
		return _pos != rhs._pos;
	}

private:
	T *next() {
		while(_pos < Service::MAX_SESSIONS) {
			T *t = static_cast<T*>(rcu_dereference(_s->_sessions[_pos]));
			if(t)
				return t;
			_pos++;
		}
		return 0;
	}
	T *prev() {
		while(_pos >= 0) {
			T *t = static_cast<T*>(rcu_dereference(_s->_sessions[_pos]));
			if(t)
				return t;
			_pos--;
		}
		return 0;
	}

	Service* _s;
	ssize_t _pos;
	T *_last;
};


template<class T>
SessionIterator<T> Service::sessions_begin() {
	return SessionIterator<T>(this);
}

template<class T>
SessionIterator<T> Service::sessions_end() {
	return SessionIterator<T>(this,MAX_SESSIONS);
}

}
