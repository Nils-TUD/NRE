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

#include <kobj/UserSm.h>
#include <util/SList.h>
#include <util/ScopedLock.h>
#include <Exception.h>
#include <String.h>

/**
 * This class keeps track of all schedulable entities in the system. Note that there is no policy
 * here. This is done by ChildManager. This class does only react on portal-calls by adding
 * SchedEntitites to a list and removing them again. Additionally, since root is the only task that
 * is allowed to create Scs, it does so as well.
 */
class Admission {
	/**
	 * Represents a scheduling entity. So, basically a global thread.
	 */
	class SchedEntity : public nre::SListItem {
	public:
		explicit SchedEntity(const nre::String &name, cpu_t cpu, capsel_t cap)
			: nre::SListItem(), _name(name), _cpu(cpu), _cap(cap),
			  _last(nre::Syscalls::sc_time(_cap)), _lastdiff() {
		}
		virtual ~SchedEntity() {
			nre::CapRange(_cap, 1, nre::Crd::OBJ_ALL).revoke(true);
			nre::CapSelSpace::get().free(_cap);
		}

		const nre::String &name() const {
			return _name;
		}
		cpu_t cpu() const {
			return _cpu;
		}
		capsel_t cap() const {
			return _cap;
		}
		timevalue_t ms_last_sec(bool update) {
			timevalue_t res = _lastdiff;
			if(update) {
				timevalue_t time = nre::Syscalls::sc_time(_cap);
				res = time - _last;
				_last = time;
			}
			_lastdiff = res;
			return res;
		}
		timevalue_t totaltime() const {
			return _last;
		}

	private:
		nre::String _name;
		cpu_t _cpu;
		capsel_t _cap;
		timevalue_t _last;
		timevalue_t _lastdiff;
	};

public:
	/**
	 * Inits this module
	 */
	static void init();

	/**
	 * Calculates the total time that has elapsed in the last second on cpu <cpu>
	 *
	 * @param cpu the logical cpu id
	 * @param update if true, the time will be requested from NOVA again and thus, a new
	 *  second is started
	 * @return the total time (in microseconds)
	 */
	static timevalue_t total_time(cpu_t cpu, bool update) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		timevalue_t total = 0;
		for(nre::SList<SchedEntity>::iterator s = _list.begin(); s != _list.end(); ++s) {
			if(s->cpu() == cpu)
				total += s->ms_last_sec(update);
		}
		return total;
	}

	/**
	 * Retrieves the properties of SchedEntity with index <idx>
	 *
	 * @param idx the index
	 * @param name will be set to the name
	 * @param cpu will be set to the CPU the Sc runs on
	 * @param time will be set to the time it has run in the last second (in microseconds)
	 * @param totaltime will be set to the total time it has run so far (in microseconds)
	 */
	static bool get_sched_entity(size_t idx, nre::String &name, cpu_t &cpu, timevalue_t &time,
	                             timevalue_t &totaltime) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		if(idx < _list.length()) {
			nre::SList<SchedEntity>::iterator s;
			for(s = _list.begin(); idx > 0 && s != _list.end(); ++s, --idx)
				;
			name = s->name();
			cpu = s->cpu();
			time = s->ms_last_sec(false);
			totaltime = s->totaltime();
			return true;
		}
		return false;
	}

	/**
	 * End-of-recursion service portal
	 */
	PORTAL static void portal_sc(capsel_t pid);

private:
	Admission();

	static void add_sc(SchedEntity *se) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		_list.append(se);
	}
	static SchedEntity *remove_sc(capsel_t sc) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		for(nre::SList<SchedEntity>::iterator it = _list.begin(); it != _list.end(); ++it) {
			if(it->cap() == sc) {
				_list.remove(&*it);
				return &*it;
			}
		}
		throw nre::Exception(nre::E_NOT_FOUND, 32, "Unable to find Sc %u", sc);
	}

	static nre::UserSm _sm;
	static nre::SList<SchedEntity> _list;
};
