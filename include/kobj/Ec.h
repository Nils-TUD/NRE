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
		TLS_SIZE	= 4
	};

public:
	enum {
		// the slot 0 is reserved for putting a ec-parameter in it
		TLS_PARAM	= 0
	};

	static Ec *current() {
		return ExecEnv::get_current_ec();
	}

protected:
	explicit Ec(cpu_t cpu,capsel_t event_base,capsel_t cap = INVALID,uintptr_t stack = 0,uintptr_t utcb = 0)
			: ObjCap(cap), _next(0), _rcu_counter(0),
			  _utcb(utcb == 0 ? new DataSpace(Utcb::SIZE,DataSpaceDesc::VIRTUAL,0) : 0),
			  _utcb_addr(utcb == 0 ? _utcb->virt() : utcb),
			  _stack(stack == 0 ? new DataSpace(ExecEnv::STACK_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW) : 0),
			  _stack_addr(stack == 0 ? _stack->virt() : stack),
			  _event_base(event_base), _cpu(cpu), _tls() {
	}
	void create(Pd *pd,Syscalls::ECType type,void *sp);

public:
	virtual ~Ec() {
		delete _stack;
		delete _utcb;
	}

	void recall() {
		Syscalls::ec_ctrl(sel(),Syscalls::RECALL);
	}

	uintptr_t stack() const {
		return _stack_addr;
	}
	capsel_t event_base() const {
		return _event_base;
	}
	Utcb *utcb() {
		return reinterpret_cast<Utcb*>(_utcb_addr);
	}
	cpu_t cpu() const {
		return _cpu;
	}

	size_t create_tls() {
		size_t next = Atomic::xadd(&_tls_idx,+1);
		assert(next < TLS_SIZE);
		return next;
	}
	template<typename T>
	T get_tls(size_t idx) const {
		assert(idx < TLS_SIZE);
		return reinterpret_cast<T>(_tls[idx]);
	}
	template<typename T>
	void set_tls(size_t idx,T val) {
		assert(idx < TLS_SIZE);
		_tls[idx] = reinterpret_cast<void*>(val);
	}

private:
	Ec(const Ec&);
	Ec& operator=(const Ec&);

	Ec *_next;
	uint32_t _rcu_counter;
	DataSpace *_utcb;
	uintptr_t _utcb_addr;
	DataSpace *_stack;
	uintptr_t _stack_addr;
	capsel_t _event_base;
	cpu_t _cpu;
	void *_tls[TLS_SIZE];
	static size_t _tls_idx;
};

}
