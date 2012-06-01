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

namespace nul {

class ServiceRegistry {
	enum {
		MAX_SERVICES	= 32
	};

public:
	class Service {
	public:
		Service(Child *child,const char *name,cpu_t cpu,capsel_t pt)
			: _child(child), _name(name), _cpu(cpu), _pt(pt) {
		}
		~Service() {
			// TODO ?
			CapSpace::get().free(_pt);
		}

		Child *child() const {
			return _child;
		}
		const char *name() const {
			return _name;
		}
		cpu_t cpu() const {
			return _cpu;
		}
		capsel_t pt() const {
			return _pt;
		}

	private:
		Child *_child;
		const char *_name;
		cpu_t _cpu;
		capsel_t _pt;
	};

	ServiceRegistry() : _srvs() {
	}

	const Service* reg(const Service& s) {
		// TODO use different exception
		if(search(s.name(),s.cpu()))
			throw Exception("Service does already exist");
		for(size_t i = 0; i < MAX_SERVICES; ++i) {
			if(_srvs[i] == 0) {
				_srvs[i] = new Service(s);
				return _srvs[i];
			}
		}
		throw Exception("All service slots in use");
	}
	void unreg(Child *child,const char *name,cpu_t cpu) {
		size_t i;
		Service *s = search(name,cpu,&i);
		if(s && s->child() == child) {
			delete _srvs[i];
			_srvs[i] = 0;
		}
	}
	const Service* find(const char *name,cpu_t cpu) const {
		return search(name,cpu);
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
	Service *search(const char *name,cpu_t cpu,size_t *idx = 0) const {
		for(size_t i = 0; i < MAX_SERVICES; ++i) {
			if(_srvs[i] && _srvs[i]->cpu() == cpu && strcmp(_srvs[i]->name(),name) == 0) {
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
