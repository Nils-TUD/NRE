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
#include <utcb/UtcbFrame.h>
#include <ScopedPtr.h>
#include <CPU.h>

namespace nul {

class Service {
	friend class ServiceInstance;

public:
	enum {
		MAX_CLIENTS	= 16
	};

	Service(const char *name,Pt::portal_func portal)
		: _caps(CapSpace::get().allocate(MAX_CLIENTS)), _sm(), _name(name), _func(portal),
		  _insts(), _clients() {
	}
	~Service() {
		for(size_t i = 0; i < MAX_CLIENTS; ++i)
			delete _clients[i];
		for(size_t i = 0; i < Hip::MAX_CPUS; ++i)
			delete _insts[i];
		CapSpace::get().free(_caps,Hip::MAX_CPUS);
	}

	const char *name() const {
		return _name;
	}
	Pt::portal_func portal() const {
		return _func;
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
	Pt *new_client() {
		ScopedLock<UserSm> guard(&_sm);
		for(size_t i = 0; i < MAX_CLIENTS; ++i) {
			if(_clients[i] == 0) {
				_clients[i] = new Pt(static_cast<LocalEc*>(Ec::current()),_caps + i,_func);
				return _clients[i];
			}
		}
		// TODO different exception
		throw Exception("No free slots");
	}

	Service(const Service&);
	Service& operator=(const Service&);

	cap_t _caps;
	UserSm _sm;
	const char *_name;
	Pt::portal_func _func;
	ServiceInstance *_insts[Hip::MAX_CPUS];
	Pt *_clients[MAX_CLIENTS];
};

}
