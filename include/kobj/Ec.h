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
#include <mem/DataSpace.h>
#include <kobj/ObjCap.h>
#include <kobj/Pd.h>
#include <kobj/UserSm.h>
#include <utcb/Utcb.h>
#include <ScopedLock.h>
#include <Syscalls.h>

namespace nul {

class RCU;
class RCULock;

class Ec : public ObjCap {
	friend class RCU;
	friend class RCULock;

	enum {
		TLS_SIZE = 4
	};

public:
	static Ec *current() {
		return ExecEnv::get_current_ec();
	}

protected:
	explicit Ec(cpu_t cpu,capsel_t event_base,capsel_t cap = INVALID,Utcb *utcb = 0,uintptr_t stack = 0)
			: ObjCap(cap), _next(0), _rcu_counter(0), _utcb(utcb),
			  _stack(ExecEnv::STACK_SIZE,DataSpace::ANONYMOUS,DataSpace::RW,0,stack),
			  _event_base(event_base), _cpu(cpu), _tls_idx(0), _tls() {
		if(!_stack.virt())
			_stack.map();
		if(!_utcb) {
			// TODO
			static UserSm sm;
			ScopedLock<UserSm> guard(&sm);
			_utcb = reinterpret_cast<Utcb*>(_utcb_addr);
			_utcb_addr -= 0x1000;
		}
	}
	void create(Pd *pd,Syscalls::ECType type,void *sp);

public:
	virtual ~Ec() {
		if(!_stack.virt())
			_stack.unmap();
		// TODO utcb
	}

	void recall() {
		Syscalls::ec_recall(sel());
	}

	const DataSpace &stack() const {
		return _stack;
	}
	capsel_t event_base() const {
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
	template<typename T>
	T *get_tls(size_t idx) const {
		assert(idx < TLS_SIZE);
		return const_cast<T*>(reinterpret_cast<const T*>(_tls[idx]));
	}
	void set_tls(size_t idx,const void *val) {
		assert(idx < TLS_SIZE);
		_tls[idx] = val;
	}

private:
	Ec(const Ec&);
	Ec& operator=(const Ec&);

	Ec *_next;
	uint32_t _rcu_counter;
	Utcb *_utcb;
	DataSpace _stack;
	capsel_t _event_base;
	cpu_t _cpu;
	size_t _tls_idx;
	const void *_tls[TLS_SIZE];

	// TODO
protected:
	static uintptr_t _utcb_addr;
};

}
