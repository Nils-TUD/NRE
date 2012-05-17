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

#include <kobj/Pt.h>
#include <subsystem/ServiceRegistry.h>

namespace nul {

// TODO wrong place?
class ElfException : public Exception {
public:
	ElfException(const char *msg) : Exception(msg) {
	}
};

class ChildManager {
	class Portals;
	friend class Portals;

	class Portals {
	public:
		enum {
			COUNT	= 7
		};

		PORTAL static void startup(cap_t pid);
		PORTAL static void initcaps(cap_t pid);
		PORTAL static void reg(cap_t pid);
		PORTAL static void unreg(cap_t pid);
		PORTAL static void getservice(cap_t pid);
		PORTAL static void map(cap_t pid);
		PORTAL static void pf(cap_t pid);
	};

	// TODO we need a data structure that allows an arbitrary number of childs or whatsoever
	enum {
		MAX_CHILDS		= 32
	};

public:
	ChildManager();
	~ChildManager();

	void load(uintptr_t addr,size_t size,const char *cmdline);
	ServiceRegistry &registry() {
		return _registry;
	}

private:
	void notify_childs(cpu_t cpu) {
		for(size_t i = 0; i < MAX_CHILDS; ++i) {
			if(_childs[i]) {
				for(uint x = 0; x < _childs[i]->_waits[cpu]; ++x)
					_childs[i]->_sms[cpu]->up();
				_childs[i]->_waits[cpu] = 0;
			}
		}
	}
	cpu_t get_cpu(cap_t pid) const {
		size_t off = (pid - _portal_caps) % per_child_caps();
		return off / Hip::get().service_caps();
	}
	Child *get_child(cap_t pid) const {
		return _childs[((pid - _portal_caps) / per_child_caps())];
	}
	void destroy_child(cap_t pid) {
		size_t i = (pid - _portal_caps) / per_child_caps();
		// TODO what if somebody is still using it?
		Child *c = _childs[i];
		_childs[i] = 0;
		_registry.remove(c);
		delete c;
	}

	static inline size_t per_child_caps() {
		return Util::nextpow2(Hip::get().service_caps() * _cpu_count);
	}

	ChildManager(const ChildManager&);
	ChildManager& operator=(const ChildManager&);

	size_t _child;
	Child *_childs[MAX_CHILDS];
	cap_t _portal_caps;
	ServiceRegistry _registry;
	UserSm _sm;
	LocalEc *_ecs[Hip::MAX_CPUS];
	LocalEc *_regecs[Hip::MAX_CPUS];
	static size_t _cpu_count;
};

}
