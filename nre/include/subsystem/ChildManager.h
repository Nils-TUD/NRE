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

#include <kobj/Pt.h>
#include <kobj/LocalThread.h>
#include <subsystem/ServiceRegistry.h>
#include <mem/DataSpaceManager.h>
#include <util/Sync.h>
#include <Exception.h>

namespace nre {

class ElfException : public Exception {
public:
	DEFINE_EXCONSTRS(ElfException)
};

class ChildException : public Exception {
public:
	DEFINE_EXCONSTRS(ChildException)
};

class ChildManager {
	class Portals;
	friend class Portals;
	friend class Child;

	class Portals {
	public:
		enum {
			COUNT	= 9
		};

		PORTAL static void startup(capsel_t pid);
		PORTAL static void init_caps(capsel_t pid);
		PORTAL static void service(capsel_t pid);
		PORTAL static void io(capsel_t pid);
		PORTAL static void sc(capsel_t pid);
		PORTAL static void gsi(capsel_t pid);
		PORTAL static void dataspace(capsel_t pid);
		PORTAL static void pf(capsel_t pid);
		PORTAL static void exception(capsel_t pid);
	};

	// TODO we need a data structure that allows an arbitrary number of childs or whatsoever
	enum {
		MAX_CHILDS		= 32,
		MAX_CMDLINE_LEN	= 256
	};
	enum ExitType {
		THREAD_EXIT,
		PROC_EXIT,
		FAULT
	};

public:
	explicit ChildManager();
	~ChildManager();

	const ServiceRegistry &registry() const {
		return _registry;
	}

	void load(uintptr_t addr,size_t size,const char *cmdline,uintptr_t main = 0,
			cpu_t cpu = CPU::current().log_id());

	void wait_for_all() {
		while(_child_count > 0)
			_diesm.zero();
	}

	void reg_service(Child *c,capsel_t cap,const String& name,const BitField<Hip::MAX_CPUS> &available) {
		ScopedLock<UserSm> guard(&_sm);
		_registry.reg(c,name,cap,1 << CPU::order(),available);
		_regsm.up();
	}

	const ServiceRegistry::Service *get_service(const String &name,bool ask_parent) {
		ScopedLock<UserSm> guard(&_sm);
		const ServiceRegistry::Service* s = registry().find(name);
		if(!s) {
			if(!ask_parent)
				throw ChildException(E_NOT_FOUND,64,"Unable to find service '%s'",name.str());
			BitField<Hip::MAX_CPUS> available;
			capsel_t caps = get_parent_service(name.str(),available);
			s = _registry.reg(0,name,caps,1 << CPU::order(),available);
			_regsm.up();
		}
		return s;
	}

	void unreg_service(Child *c,const String& name) {
		ScopedLock<UserSm> guard(&_sm);
		_registry.unreg(c,name);
	}

private:
	size_t free_slot() const {
		ScopedLock<UserSm> guard(&_slotsm);
		for(size_t i = 0; i < MAX_CHILDS; ++i) {
			if(_childs[i] == 0)
				return i;
		}
		throw ChildException(E_CAPACITY,"No free child slots");
	}

	static inline size_t per_child_caps() {
		return Math::next_pow2(Hip::get().service_caps() * CPU::count());
	}
	cpu_t get_cpu(capsel_t pid) const {
		size_t off = (pid - _portal_caps) % per_child_caps();
		return off / Hip::get().service_caps();
	}
	Child *get_child(capsel_t pid) const {
		Child *c = rcu_dereference(_childs[((pid - _portal_caps) / per_child_caps())]);
		if(!c)
			throw ChildException(E_NOT_FOUND,32,"Child with portal %u does not exist",pid);
		return c;
	}
	uint get_vector(capsel_t pid) const {
		return (pid - _portal_caps) % Hip::get().service_caps();
	}
	void term_child(capsel_t pid,UtcbExcFrameRef &uf);
	void kill_child(capsel_t pid,UtcbExcFrameRef &uf,ExitType type);
	void destroy_child(capsel_t pid);

	static void prepare_stack(Child *c,uintptr_t &sp,uintptr_t csp);

	capsel_t get_parent_service(const char *name,BitField<Hip::MAX_CPUS> &available);
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
	mutable UserSm _slotsm;
	Sm _regsm;
	Sm _diesm;
	// we need different Ecs to be able to receive a different number of caps
	LocalThread **_ecs;
	LocalThread **_regecs;
};

}
