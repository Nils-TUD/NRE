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

#include <kobj/Pd.h>
#include <kobj/GlobalThread.h>
#include <kobj/Sc.h>
#include <kobj/Pt.h>
#include <kobj/UserSm.h>
#include <kobj/Gsi.h>
#include <kobj/Ports.h>
#include <subsystem/ChildMemory.h>
#include <mem/RegionManager.h>
#include <utcb/UtcbFrame.h>
#include <util/BitField.h>
#include <CPU.h>
#include <util/Atomic.h>

namespace nre {

class OStream;
class Child;
class ChildManager;

OStream &operator<<(OStream &os,const Child &c);

class Child {
	friend class ChildManager;
	friend OStream &operator<<(OStream &os,const Child &c);

public:
	const String &cmdline() const {
		return _cmdline;
	}
	unsigned refs() const {
		return _refs;
	}
	ChildMemory &reglist() {
		return _regs;
	}
	const ChildMemory &reglist() const {
		return _regs;
	}
	RegionManager &io() {
		return _io;
	}
	const RegionManager &io() const {
		return _io;
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
	BitField<Hip::MAX_GSIS> &gsis() {
		return _gsis;
	}
	const BitField<Hip::MAX_GSIS> &gsis() const {
		return _gsis;
	}

private:
	explicit Child(ChildManager *cm,const char *cmdline)
			: _cm(cm), _cmdline(cmdline), _refs(1), _started(), _pd(), _ec(), _sc(), _pts(),
			  _ptcount(), _regs(), _io(), _entry(), _main(), _stack(), _utcb(), _hip(),
			  _last_fault_addr(), _last_fault_cpu(), _gsis(),
			  _gsi_caps(CapSpace::get().allocate(Hip::MAX_GSIS)), _gsi_next(), _sm() {
	}
	~Child() {
		for(size_t i = 0; i < _ptcount; ++i)
			delete _pts[i];
		delete[] _pts;
		if(_sc)
			delete _sc;
		if(_ec)
			delete _ec;
		if(_pd)
			delete _pd;
		release_gsis();
		release_ports();
		CapSpace::get().free(_gsi_caps,Hip::MAX_GSIS);
	}

	void increase_refs() {
		Atomic::add(&_refs,+1);
	}
	void decrease_refs() {
		Atomic::add(&_refs,-1);
	}

	void release_gsis() {
		UtcbFrame uf;
		for(uint i = 0; i < Hip::MAX_GSIS; ++i) {
			if(_gsis.is_set(i)) {
				uf << Gsi::RELEASE << i;
				CPU::current().gsi_pt->call(uf);
				uf.clear();
			}
		}
	}

	void release_ports() {
		UtcbFrame uf;
		for(RegionManager::iterator it = _io.begin(); it != _io.end(); ++it) {
			if(it->size) {
				uf << Ports::RELEASE << it->addr << it->size;
				CPU::current().io_pt->call(uf);
				uf.clear();
			}
		}
	}

	Child(const Child&);
	Child& operator=(const Child&);

	ChildManager *_cm;
	String _cmdline;
	unsigned _refs;
	bool _started;
	Pd *_pd;
	GlobalThread *_ec;
	Sc *_sc;
	Pt **_pts;
	size_t _ptcount;
	ChildMemory _regs;
	RegionManager _io;
	uintptr_t _entry;
	uintptr_t _main;
	uintptr_t _stack;
	uintptr_t _utcb;
	uintptr_t _hip;
	uintptr_t _last_fault_addr;
	cpu_t _last_fault_cpu;
	BitField<Hip::MAX_GSIS> _gsis;
	capsel_t _gsi_caps;
	capsel_t _gsi_next;
	UserSm _sm;
};

}
