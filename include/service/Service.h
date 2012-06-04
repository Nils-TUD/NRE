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

class ClientData {
public:
	ClientData(capsel_t srvpt,capsel_t dspt,Pt::portal_func func)
		: _srvpt(static_cast<LocalEc*>(Ec::current()),srvpt,func),
		  _dspt(static_cast<LocalEc*>(Ec::current()),dspt,portal_ds), _ds() {
	}
	virtual ~ClientData() {
		delete _ds;
	}

	Pt &pt() {
		return _srvpt;
	}
	const DataSpace *ds() const {
		return _ds;
	}

private:
	PORTAL static void portal_ds(capsel_t pid);

	Pt _srvpt;
	Pt _dspt;
	DataSpace *_ds;
};

class Service {
	friend class ServiceInstance;

public:
	enum {
		MAX_CLIENTS	= 16
	};

	Service(const char *name,Pt::portal_func portal)
		// aligned by 2 because we give out two portals at once
		: _caps(CapSpace::get().allocate(MAX_CLIENTS * 2,2)), _sm(), _name(name), _func(portal),
		  _insts(), _clients() {
	}
	virtual ~Service() {
		for(size_t i = 0; i < MAX_CLIENTS; ++i)
			delete _clients[i];
		for(size_t i = 0; i < Hip::MAX_CPUS; ++i)
			delete _insts[i];
		CapSpace::get().free(_caps,MAX_CLIENTS);
	}

	const char *name() const {
		return _name;
	}
	Pt::portal_func portal() const {
		return _func;
	}

	template<class T>
	T *get_client(capsel_t pid) {
		return static_cast<T*>(_clients[(pid - _caps) / 2]);
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
	virtual ClientData *create_client(capsel_t srvpt,capsel_t dspt,Pt::portal_func func) {
		return new ClientData(srvpt,dspt,func);
	}

	ClientData *new_client() {
		ScopedLock<UserSm> guard(&_sm);
		for(size_t i = 0; i < MAX_CLIENTS; ++i) {
			if(_clients[i] == 0) {
				_clients[i] = create_client(_caps + i * 2,_caps + i * 2 + 1,_func);
				return _clients[i];
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
	ClientData *_clients[MAX_CLIENTS];
};

}
