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

#include <mem/RegionList.h>
#include <kobj/Pd.h>
#include <kobj/GlobalEc.h>
#include <kobj/Sc.h>
#include <kobj/Pt.h>
#include <kobj/UserSm.h>

namespace nul {

class ChildManager;

class Child {
	friend class ChildManager;

	Child(const char *cmdline) : _cmdline(cmdline), _started(), _pd(), _ec(), _sc(), _pts(),
			_ptcount(), _regs(), _entry(), _stack(), _utcb(), _hip(), _sm() {
	}
	~Child() {
		if(_pd)
			delete _pd;
		if(_ec)
			delete _ec;
		if(_sc)
			delete _sc;
		for(size_t i = 0; i < _ptcount; ++i)
			delete _pts[i];
		delete[] _pts;
	}

public:
	const char *cmdline() const {
		return _cmdline;
	}
	RegionList &reglist() {
		return _regs;
	}
	const RegionList &reglist() const {
		return _regs;
	}
	uintptr_t entry() const {
		return _entry;
	}
	uintptr_t stack() const {
		return _stack;
	}
	uintptr_t utcb() const {
		return _utcb;
	}
	uintptr_t hip() const {
		return _hip;
	}

private:
	const char *_cmdline;
	bool _started;
	Pd *_pd;
	GlobalEc *_ec;
	Sc *_sc;
	Pt **_pts;
	size_t _ptcount;
	RegionList _regs;
	uintptr_t _entry;
	uintptr_t _stack;
	uintptr_t _utcb;
	uintptr_t _hip;
	UserSm _sm;
};

}
