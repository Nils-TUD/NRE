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

#include <arch/ExecEnv.h>
#include <cap/CapHolder.h>
#include <kobj/KObject.h>
#include <kobj/Pd.h>
#include <utcb/Utcb.h>
#include <utcb/UtcbExc.h>
#include <Syscalls.h>

namespace nul {

class Ec : public KObject {
	enum {
		TLS_SIZE = 4
	};

public:
	static Ec *current() {
		return ExecEnv::get_current_ec();
	}

protected:
	explicit Ec(cpu_t cpu,cap_t event_base,cap_t cap = INVALID,Utcb *utcb = 0)
			: KObject(cap), _utcb(utcb), _stack(), _event_base(event_base), _cpu(cpu),
			  _tls_idx(0), _tls() {
		if(!_utcb) {
			// TODO
			_utcb = reinterpret_cast<Utcb*>(_utcb_addr);
			_utcb_addr -= 0x1000;
		}
	}
	void create(Pd *pd,Syscalls::ECType type,void *sp) {
		CapHolder ch;
		Syscalls::create_ec(ch.get(),_utcb,sp,_cpu,_event_base,type,pd->cap());
		cap(ch.release());
		_stack = reinterpret_cast<uintptr_t>(sp) & ~(ExecEnv::PAGE_SIZE - 1);
	}

public:
	virtual ~Ec() {
		// TODO stack, utcb
	}

	uintptr_t stack() const {
		return _stack;
	}
	cap_t event_base() const {
		return _event_base;
	}
	Utcb *utcb() {
		return _utcb;
	}
	cpu_t cpu() const {
		return _cpu;
	}

	size_t create_tls() {
		assert(_tls_idx + 1 < TLS_SIZE);
		return _tls_idx++;
	}
	void *get_tls(size_t idx) const {
		if(idx >= TLS_SIZE)
			return 0;
		return const_cast<void*>(_tls[idx]);
	}
	void set_tls(size_t idx,const void *val) {
		if(idx < TLS_SIZE)
			_tls[idx] = val;
	}

private:
	Ec(const Ec&);
	Ec& operator=(const Ec&);

	Utcb *_utcb;
	uintptr_t _stack;
	cap_t _event_base;
	cpu_t _cpu;
	size_t _tls_idx;
	const void *_tls[TLS_SIZE];

	// TODO
protected:
	static uintptr_t _utcb_addr;
};

}
