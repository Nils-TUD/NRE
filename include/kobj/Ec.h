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
#include <mem/DataSpace.h>
#include <kobj/ObjCap.h>
#include <kobj/Pd.h>
#include <kobj/UserSm.h>
#include <utcb/Utcb.h>
#include <ScopedLock.h>
#include <ScopedCapSels.h>
#include <Syscalls.h>

namespace nul {

class RCU;
class RCULock;

/**
 * Represents an execution context. This class can't be used directly. To create an Ec, use LocalEc
 * or GlobalEc. Note that each Ec contains a few slots for thread local storage (TLS). The index
 * Ec::TLS_PARAM is always available, e.g. to pass a parameter to an Ec. You may create additional
 * ones by Ec::create_tls().
 */
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

	/**
	 * @return the current execution context
	 */
	static Ec *current() {
		return ExecEnv::get_current_ec();
	}

protected:
	/**
	 * Constructor
	 *
	 * @param cpu the cpu to bind the Ec to
	 * @param evb the offset for the event-portals
	 * @param cap the capability (INVALID if a new one should be used)
	 * @param stack the stack address (0 = create one automatically)
	 * @param uaddr the utcb address (0 = create one automatically)
	 */
	explicit Ec(cpu_t cpu,capsel_t evb,capsel_t cap = INVALID,uintptr_t stack = 0,uintptr_t uaddr = 0)
			: ObjCap(cap), _next(0), _rcu_counter(0),
			  _utcb(uaddr == 0 ? new DataSpace(Utcb::SIZE,DataSpaceDesc::VIRTUAL,0) : 0),
			  _utcb_addr(uaddr == 0 ? _utcb->virt() : uaddr),
			  _stack(stack == 0 ? new DataSpace(ExecEnv::STACK_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW) : 0),
			  _stack_addr(stack == 0 ? _stack->virt() : stack),
			  _event_base(evb), _cpu(cpu), _tls() {
	}
	/**
	 * The actual creation of the Ec.
	 *
	 * @param pd the protection domain the Ec should run in
	 * @param type the type of Ec
	 * @param sp the stack-pointer
	 */
	void create(Pd *pd,Syscalls::ECType type,void *sp);

public:
	/**
	 * Destructor
	 */
	virtual ~Ec() {
		delete _stack;
		delete _utcb;
	}

	/**
	 * Let's this Ec perform a recall
	 */
	void recall() {
		Syscalls::ec_ctrl(sel(),Syscalls::RECALL);
	}

	/**
	 * @return the cpu
	 */
	cpu_t cpu() const {
		return _cpu;
	}
	/**
	 * @return the event-base
	 */
	capsel_t event_base() const {
		return _event_base;
	}
	/**
	 * @return the stack-address
	 */
	uintptr_t stack() const {
		return _stack_addr;
	}
	/**
	 * @return the utcb
	 */
	Utcb *utcb() {
		return reinterpret_cast<Utcb*>(_utcb_addr);
	}

	/**
	 * Creates a new TLS slot for all Ecs
	 *
	 * @return the TLS index
	 */
	size_t create_tls() {
		size_t next = Atomic::xadd(&_tls_idx,+1);
		assert(next < TLS_SIZE);
		return next;
	}
	/**
	 * @param idx the TLS index
	 * @return the value at given index
	 */
	template<typename T>
	T get_tls(size_t idx) const {
		assert(idx < TLS_SIZE);
		return reinterpret_cast<T>(_tls[idx]);
	}
	/**
	 * @param idx the TLS index
	 * @param val the new value
	 */
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
