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

#include <util/SList.h>
#include <Syscalls.h>
#include <String.h>

class TimeUser : public nre::SListItem {
public:
	explicit TimeUser(const nre::String &name,cpu_t cpu,capsel_t sc,bool idle)
		: nre::SListItem(), _name(name), _cpu(cpu), _sc(sc), _idle(idle),
		  _last(nre::Syscalls::sc_time(_sc)), _lastdiff() {
	}

	const nre::String &name() const {
		return _name;
	}
	cpu_t cpu() const {
		return _cpu;
	}
	capsel_t cap() const {
		return _sc;
	}
	bool idle() const {
		return _idle;
	}
	timevalue_t ms_last_sec(bool update) {
		timevalue_t res = _lastdiff;
		if(update) {
			timevalue_t time = nre::Syscalls::sc_time(_sc);
			res = time - _last;
			_last = time;
		}
		_lastdiff = res;
		return res;
	}

private:
	nre::String _name;
	cpu_t _cpu;
	capsel_t _sc;
	bool _idle;
	timevalue_t _last;
	timevalue_t _lastdiff;
};
