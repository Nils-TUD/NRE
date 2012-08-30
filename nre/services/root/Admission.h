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
#include <services/TimeTracker.h>
#include <util/SList.h>
#include <util/ScopedLock.h>
#include <Exception.h>
#include <String.h>

class Admission {
	class SchedEntity : public nre::SListItem {
	public:
		explicit SchedEntity(const nre::String &name,cpu_t cpu,capsel_t cap)
			: nre::SListItem(), _name(name), _cpu(cpu), _cap(cap) {
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

	private:
		nre::String _name;
		cpu_t _cpu;
		capsel_t _cap;
	};

public:
	PORTAL static void portal_sc(capsel_t pid);

private:
	Admission();

	static void notify_start(const nre::String &name,cpu_t cpu,capsel_t sc);
	static void notify_stop(cpu_t cpu,capsel_t sc);

	static void add_sc(SchedEntity *se) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		notify_start(se->name(),se->cpu(),se->cap());
		_list.append(se);
	}
	static SchedEntity *remove_sc(capsel_t sc) {
		nre::ScopedLock<nre::UserSm> guard(&_sm);
		for(nre::SList<SchedEntity>::iterator it = _list.begin(); it != _list.end(); ++it) {
			if(it->cap() == sc) {
				_list.remove(&*it);
				notify_stop(it->cpu(),sc);
				return &*it;
			}
		}
		throw nre::Exception(nre::E_NOT_FOUND,"Unable to find Sc");
	}

	static nre::Connection *_tt_con;
	static nre::TimeTrackerSession *_tt_sess;
	static nre::UserSm _sm;
	static nre::SList<SchedEntity> _list;
};
