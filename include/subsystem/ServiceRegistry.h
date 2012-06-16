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

#include <subsystem/Child.h>
#include <BitField.h>
#include <Exception.h>

namespace nul {

class ServiceRegistryException : public Exception {
public:
	explicit ServiceRegistryException(ErrorCode code) throw() : Exception(code) {
	}
};

class ServiceRegistry {
	enum {
		MAX_SERVICES	= 32
	};

public:
	class Service {
	public:
		Service(Child *child,const String &name,capsel_t pts,const BitField<Hip::MAX_CPUS> &available)
			: _child(child), _name(name), _pts(pts), _available(available) {
		}
		~Service() {
			// TODO ?
		}

		Child *child() const {
			return _child;
		}
		const String &name() const {
			return _name;
		}
		const BitField<Hip::MAX_CPUS> &available() const {
			return _available;
		}
		capsel_t pts() const {
			return _pts;
		}

	private:
		Child *_child;
		String _name;
		capsel_t _pts;
		BitField<Hip::MAX_CPUS> _available;
	};

	ServiceRegistry() : _srvs() {
	}

	const Service* reg(const Service& s) {
		// TODO use different exception
		if(search(s.name()))
			throw ServiceRegistryException(E_EXISTS);
		for(size_t i = 0; i < MAX_SERVICES; ++i) {
			if(_srvs[i] == 0) {
				_srvs[i] = new Service(s);
				return _srvs[i];
			}
		}
		throw ServiceRegistryException(E_CAPACITY);
	}
	void unreg(Child *child,const String &name) {
		size_t i;
		Service *s = search(name,&i);
		if(s && s->child() == child) {
			delete _srvs[i];
			_srvs[i] = 0;
		}
	}
	const Service* find(const String &name) const {
		return search(name);
	}
	void remove(Child *child) {
		for(size_t i = 0; i < MAX_SERVICES; ++i) {
			if(_srvs[i] && _srvs[i]->child() == child) {
				delete _srvs[i];
				_srvs[i] = 0;
			}
		}
	}

private:
	Service *search(const String &name,size_t *idx = 0) const {
		for(size_t i = 0; i < MAX_SERVICES; ++i) {
			if(_srvs[i] && strcmp(_srvs[i]->name().str(),name.str()) == 0) {
				if(idx)
					*idx = i;
				return _srvs[i];
			}
		}
		return 0;
	}

	Service *_srvs[MAX_SERVICES];
};

}
