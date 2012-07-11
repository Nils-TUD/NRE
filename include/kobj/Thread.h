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
#include <kobj/Ec.h>
#include <mem/DataSpace.h>
#include <utcb/Utcb.h>
#include <util/Atomic.h>
#include <Syscalls.h>

namespace nre {

class Pd;
class RCU;
class RCULock;

/**
 * Represents a thread, i.e. an Ec that has a stack and a Utcb. It is the base class for the two
 * supported thread variants, LocalThread and GlobalThread. This class can't be used directly.
 * Note that each Thread contains a few slots for thread local storage (TLS). The index
 * Thread::TLS_PARAM is always available, e.g. to pass a parameter to a Thread. You may create
 * additional ones by Thread::create_tls().
 */
class Thread : public Ec {
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
	static Thread *current() {
		return ExecEnv::get_current_thread();
	}

protected:
	/**
	 * Constructor
	 *
	 * @param cpu the logical cpu to bind the Thread to
	 * @param evb the offset for the event-portals
	 * @param cap the capability (INVALID if a new one should be used)
	 * @param stack the stack address (0 = create one automatically)
	 * @param uaddr the utcb address (0 = create one automatically)
	 */
	explicit Thread(cpu_t cpu,capsel_t evb,capsel_t cap = INVALID,uintptr_t stack = 0,uintptr_t uaddr = 0)
			: Ec(cpu,evb,cap), _next(0), _rcu_counter(0),
			  _utcb(uaddr == 0 ? new DataSpace(Utcb::SIZE,DataSpaceDesc::VIRTUAL,0) : 0),
			  _utcb_addr(uaddr == 0 ? _utcb->virt() : uaddr),
			  _stack(stack == 0 ? new DataSpace(ExecEnv::STACK_SIZE,DataSpaceDesc::ANONYMOUS,DataSpaceDesc::RW) : 0),
			  _stack_addr(stack == 0 ? _stack->virt() : stack),
			  _tls() {
	}
	/**
	 * The actual creation of the Thread.
	 *
	 * @param pd the protection domain the Thread should run in
	 * @param type the type of Thread
	 * @param sp the stack-pointer
	 */
	void create(Pd *pd,Syscalls::ECType type,void *sp);

public:
	/**
	 * Destructor
	 */
	virtual ~Thread() {
		delete _stack;
		delete _utcb;
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
	 * Creates a new TLS slot for all Threads
	 *
	 * @return the TLS index
	 */
	size_t create_tls() {
		size_t next = Atomic::add(&_tls_idx,+1);
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
	Thread(const Thread&);
	Thread& operator=(const Thread&);

	Thread *_next;
	uint32_t _rcu_counter;
	DataSpace *_utcb;
	uintptr_t _utcb_addr;
	DataSpace *_stack;
	uintptr_t _stack_addr;
	void *_tls[TLS_SIZE];
	static size_t _tls_idx;
};

}
