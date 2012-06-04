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

#include <kobj/LocalEc.h>
#include <kobj/Pt.h>
#include <kobj/Sm.h>
#include <kobj/UserSm.h>
#include <service/ServiceInstance.h>
#include <mem/DataSpace.h>
#include <utcb/UtcbFrame.h>
#include <Exception.h>
#include <ScopedPtr.h>
#include <CPU.h>

namespace nul {

class ServiceException : public Exception {
public:
	explicit ServiceException(ErrorCode code) throw() : Exception(code) {
	}
};

class SessionData {
	friend class ServiceInstance;

public:
	SessionData(capsel_t pt,Pt::portal_func func)
		: _pt(static_cast<LocalEc*>(Ec::current()),pt,func), _ds() {
	}
	virtual ~SessionData() {
		delete _ds;
	}

	Pt &pt() {
		return _pt;
	}
	const DataSpace *ds() const {
		return _ds;
	}

private:
	Pt _pt;
	DataSpace *_ds;
};

class Service {
	friend class ServiceInstance;

public:
	enum Command {
		OPEN_SESSION,
		SHARE_DATASPACE
	};

	enum {
		MAX_SESSIONS_SHIFT	= 5,
		MAX_SESSIONS		= 1 << MAX_SESSIONS_SHIFT
	};

	Service(const char *name,Pt::portal_func portal)
		: _caps(CapSpace::get().allocate(MAX_SESSIONS,MAX_SESSIONS)), _sm(), _name(name), _func(portal),
		  _insts(), _sessions() {
	}
	virtual ~Service() {
		for(size_t i = 0; i < MAX_SESSIONS; ++i)
			delete _sessions[i];
		for(size_t i = 0; i < Hip::MAX_CPUS; ++i)
			delete _insts[i];
		CapSpace::get().free(_caps,MAX_SESSIONS);
	}

	const char *name() const {
		return _name;
	}
	Pt::portal_func portal() const {
		return _func;
	}

	template<class T>
	T *get_session(capsel_t pid) {
		return static_cast<T*>(_sessions[pid - _caps]);
	}

	void reg(cpu_t cpu) {
		assert(_insts[cpu] == 0);
		_insts[cpu] = new ServiceInstance(this,cpu);
	}
	void wait() {
		Sm sm(0);
		sm.down();
	}

private:
	virtual SessionData *create_session(capsel_t pt,Pt::portal_func func) {
		return new SessionData(pt,func);
	}

	SessionData *new_session() {
		ScopedLock<UserSm> guard(&_sm);
		for(size_t i = 0; i < MAX_SESSIONS; ++i) {
			if(_sessions[i] == 0) {
				_sessions[i] = create_session(_caps + i,_func);
				return _sessions[i];
			}
		}
		throw ServiceException(E_CAPACITY);
	}

	Service(const Service&);
	Service& operator=(const Service&);

	capsel_t _caps;
	UserSm _sm;
	const char *_name;
	Pt::portal_func _func;
	ServiceInstance *_insts[Hip::MAX_CPUS];
	SessionData *_sessions[MAX_SESSIONS];
};

}
