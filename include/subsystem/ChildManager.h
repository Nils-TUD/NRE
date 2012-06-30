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
#include <kobj/LocalEc.h>
#include <subsystem/ServiceRegistry.h>
#include <mem/DataSpaceManager.h>
#include <Exception.h>

namespace nul {

class ElfException : public Exception {
public:
	ElfException(ErrorCode code) : Exception(code) {
	}
};

class ChildException : public Exception {
public:
	ChildException(ErrorCode code) : Exception(code) {
	}
};

class ChildManager {
	class Portals;
	friend class Portals;

	class Portals {
	public:
		enum {
			COUNT	= 9
		};

		PORTAL static void startup(capsel_t pid);
		PORTAL static void init_caps(capsel_t pid);
		PORTAL static void reg(capsel_t pid);
		PORTAL static void unreg(capsel_t pid);
		PORTAL static void get_service(capsel_t pid);
		PORTAL static void io(capsel_t);
		PORTAL static void gsi(capsel_t);
		PORTAL static void dataspace(capsel_t pid);
		PORTAL static void pf(capsel_t pid);
	};

	// TODO we need a data structure that allows an arbitrary number of childs or whatsoever
	enum {
		MAX_CHILDS		= 32,
		MAX_CMDLINE_LEN	= 256
	};

public:
	explicit ChildManager();
	~ChildManager();

	ServiceRegistry &registry() {
		return _registry;
	}

	void load(uintptr_t addr,size_t size,const char *cmdline,uintptr_t main = 0);

	void wait_for_all() {
		while(_child_count > 0)
			_diesm.zero();
	}

	void reg_service(Child *c,capsel_t cap,const String& name,const BitField<Hip::MAX_CPUS> &available) {
		ScopedLock<UserSm> guard(&_sm);
		registry().reg(ServiceRegistry::Service(c,name,cap,available));
		_regsm.up();
	}

	const ServiceRegistry::Service *get_service(const String &name) {
		ScopedLock<UserSm> guard(&_sm);
		const ServiceRegistry::Service* s = registry().find(name);
		if(!s) {
			BitField<Hip::MAX_CPUS> available;
			capsel_t caps = get_parent_service(name.str(),available);
			s = registry().reg(ServiceRegistry::Service(0,name,caps,available));
			_regsm.up();
		}
		return s;
	}

	void unreg_service(Child *c,const String& name) {
		ScopedLock<UserSm> guard(&_sm);
		registry().unreg(c,name);
	}

private:
	size_t free_slot() const {
		for(size_t i = 0; i < MAX_CHILDS; ++i) {
			if(_childs[i] == 0)
				return i;
		}
		return MAX_CHILDS;
	}

	cpu_t get_cpu(capsel_t pid) const {
		size_t off = (pid - _portal_caps) % per_child_caps();
		return off / Hip::get().service_caps();
	}
	Child *get_child(capsel_t pid) const {
		Child *c = _childs[((pid - _portal_caps) / per_child_caps())];
		if(!c)
			throw ChildException(E_NOT_FOUND);
		return c;
	}
	void destroy_child(capsel_t pid) {
		size_t i = (pid - _portal_caps) / per_child_caps();
		Child *c = _childs[i];
		c->decrease_refs();
		if(c->refs() == 0) {
			Serial::get() << "Destroying child '" << c->cmdline() << "'\n";
			// note that we're safe here because we only get here if there is only one Ec left and
			// this one has just caused a fault. thus, there can't be somebody else using this
			// client instance
			_childs[i] = 0;
			_registry.remove(c);
			delete c;
			_child_count--;
			Sync::memory_barrier();
			_diesm.up();
		}
	}
	capsel_t get_parent_service(const char *name,BitField<Hip::MAX_CPUS> &available);

	static inline size_t per_child_caps() {
		return Math::next_pow2(Hip::get().service_caps() * CPU::count());
	}
	static void prepare_stack(Child *c,uintptr_t &sp,uintptr_t csp);

	void map(UtcbFrameRef &uf,Child *c,DataSpace::RequestType type);
	void switch_to(UtcbFrameRef &uf,Child *c);
	void unmap(UtcbFrameRef &uf,Child *c);

	ChildManager(const ChildManager&);
	ChildManager& operator=(const ChildManager&);

	size_t _child_count;
	Child *_childs[MAX_CHILDS];
	capsel_t _portal_caps;
	DataSpaceManager<DataSpace> _dsm;
	ServiceRegistry _registry;
	UserSm _sm;
	UserSm _switchsm;
	Sm _regsm;
	Sm _diesm;
	// we need different Ecs to be able to receive a different number of caps
	LocalEc **_ecs;
	LocalEc **_regecs;
};

}
