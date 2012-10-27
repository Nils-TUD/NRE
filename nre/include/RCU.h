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

#include <kobj/Thread.h>
#include <kobj/UserSm.h>
#include <util/ScopedLock.h>
#include <util/Sync.h>
#include <util/Util.h>
#include <util/SList.h>

/**
 * Usage:
 * Whenever you access a RCUObject, use rcu_dereference() to ensure that the compiler does not
 * break things. And note that the received object may change! So, if you want to work with an object
 * load it once with rcu_dereference() into a variable and use this variable for the rest of your
 * work. Whenever you change a pointer to a RCUObject, use rcu_assign_pointer() for the
 * same reason. If you want to delete a RCUObject, use rcu_assign_pointer(ptr,0) before calling
 * RCU::invalidate(ptr). This is important because the method RCU::invalidate() is based on the
 * assumption that whenever an object is invalidated, there is NO way anymore to get access to it.
 *
 * Please note that RCU::invalidate() doesn't ensure that all objects are deleted. That is, only
 * objects that are already safe to delete, are deleted. This way, there is no busy-waiting
 * going on until the deletion is possible. If that is important for you, you can use RCU::gc(true)
 * to force the method to wait until all objects can be deleted.
 */

/*
 * The following macros are heavily based on libsync (http://www.ioremap.net/node/224)
 */

/*
 * Prevent the compiler from merging or refetching accesses.  The compiler
 * is also forbidden from reordering successive instances of ACCESS_ONCE(),
 * but only when the compiler is aware of some particular ordering.  One way
 * to make the compiler aware of ordering is to put the two invocations of
 * ACCESS_ONCE() in different C statements.
 *
 * This macro does absolutely -nothing- to prevent the CPU from reordering,
 * merging, or refetching absolutely anything at any time.  Its main intended
 * use is to mediate communication between process-level code and irq/NMI
 * handlers, all running on the same CPU.
 */
#define ACCESS_ONCE(x)              (*(volatile __typeof__(x) *) & (x))

/*
 * Identify a shared load. A smp_rmc() or smp_mc() should come before the load.
 */
#define _LOAD_SHARED(p)             ACCESS_ONCE(p)

/*
 * Load a data from shared memory, doing a cache flush if required.
 */
#define LOAD_SHARED(p)                    \
	({                                    \
		Sync::memory_barrier();           \
		_LOAD_SHARED(p);                  \
	})

/*
 * Identify a shared store. A smp_wmc() or smp_mc() should follow the store.
 */
#define _STORE_SHARED(x, v)               \
	do {                                  \
		(x) = (v);                        \
	} while(0)

/*
 * Store v into x, where x is located in shared memory. Performs the required
 * cache flush after writing.
 */
#define STORE_SHARED(x, v)                \
	do {                                  \
		_STORE_SHARED(x, v);              \
		Sync::memory_barrier();           \
	} while(0)

/**
 * rcu_dereference - fetch an RCU-protected pointer in an
 * RCU read-side critical section.  This pointer may later
 * be safely dereferenced.
 *
 * Documents exactly which pointers are protected by RCU.
 */
#define rcu_dereference(p)          LOAD_SHARED(p)

/**
 * rcu_assign_pointer - assign (publicize) a pointer to a newly
 * initialized structure that will be dereferenced by RCU read-side
 * critical sections.  Returns the value assigned.
 *
 * Prevents the compiler from reordering the code that initializes the
 * structure after the pointer assignment.  More importantly, this
 * call documents which pointers will be dereferenced by RCU read-side
 * code.
 */
#define rcu_assign_pointer(p, v)    STORE_SHARED(p, v)

namespace nre {

class RCU;

/**
 * A class to mark RCU read sections. Usage:
 * {
 *   ScopedLock<RCULock> guard(&RCU::lock());
 *   // do stuff
 * }
 */
class RCULock {
public:
	explicit RCULock() {
	}

	void down() {
		Thread *cur = Thread::current();
		uint32_t counter = cur->_rcu_counter;
		// update version-counter if we're entering a critical section
		if(!(counter & 0xFFFF))
			counter += 0x10000;
		// always update the nested-counter
		counter++;
		// ensure that the counter-increase is written before anything else in the critical section
		cur->_rcu_counter = counter;
		Sync::memory_barrier();
	}
	void up() {
		// ensure that everything in the critical section is written before the counter is increased
		Sync::memory_barrier();
		Thread *cur = Thread::current();
		cur->_rcu_counter--;
	}

private:
	RCULock(const RCULock&);
	RCULock& operator=(const RCULock&);
};

/**
 * The base-class for all objects that are handled by RCU
 */
class RCUObject {
	friend class RCU;

	enum State {
		VALID,
		INVALID,
		DELETABLE
	};

public:
	explicit RCUObject() : _state(VALID), _next(0) {
	}
	virtual ~RCUObject() {
	}

	bool valid() const {
		return _state == VALID;
	}

private:
	State _state;
	RCUObject *_next;
};

class RCU {
public:
	/**
	 * Adds the given Thread to the list of known Threads. Will be called by the Thread class
	 * automatically.
	 */
	static void add(Thread *ec) {
		ScopedLock<UserSm> guard(&_ecsm);
		_ecs.append(ec);
	}
	/**
	 * Removes the given thread from the list of known Threads. Will be called by the Thread class
	 * automatically.
	 */
	static void remove(Thread *ec) {
		ScopedLock<UserSm> guard(&_ecsm);
		_ecs.remove(ec);
	}

	/**
	 * Marks the given object as deletable. It assumes that you already made sure that nobody can
	 * get access to it anymore, i.e. that there is no pointer to that object anymore.
	 * The method will also delete objects from previous invalidate() calls, if it is now safe to
	 * delete them.
	 */
	static void invalidate(RCUObject *o) {
		ScopedLock<UserSm> guard(&_sm);
		o->_state = RCUObject::INVALID;
		o->_next = _objs;
		_objs = o;

		// check if all Ecs have left the critical section or entered it another time since the
		// last invalidate call. if so, the last invalidated object is deletable (and all following
		// ones as well)
		if(o->_next && deletable()) {
			o->_next->_state = RCUObject::DELETABLE;
			delete_objects();
		}

		store_versions();
	}

	/**
	 * Performs a garbage-collection. That is, all objects that are safe to delete, are deleted now.
	 * If you set force to true, the method uses busy-waiting until all objects can be deleted.
	 */
	static void gc(bool force) {
		ScopedLock<UserSm> guard(&_sm);
		if(!_objs)
			return;

		if(force) {
			// wait until all objects are deletable
			while(!deletable())
				Util::pause();
			_objs->_state = RCUObject::DELETABLE;
		}
		// check if the last invalidated object is deletable as well
		else if(deletable())
			_objs->_state = RCUObject::DELETABLE;

		delete_objects();
	}

	/**
	 * Use this "lock" (with ScopedLock<RCULock>) to mark rcu read sections.
	 */
	static RCULock &lock() {
		return _lock;
	}

private:
	static void delete_objects() {
		bool deletable = false;
		RCUObject *p = 0, *o = _objs;
		while(o != 0) {
			// all objects behind the first deletable one are deletable
			if(!deletable && o->_state != RCUObject::DELETABLE) {
				p = o;
				o = o->_next;
				continue;
			}

			deletable = true;
			RCUObject *n = o->_next;
			delete o;
			if(p)
				p->_next = n;
			else
				_objs = n;
			o = n;
		}
	}

	static bool deletable() {
		// if new Ecs have been added since the last invalidation, we don't know whether this one
		// is currently accessing an object. So, store all version-numbers again.
		if(_versions_count != _ecs.length())
			store_versions();

		ScopedLock<UserSm> guard(&_ecsm);
		size_t i = 0;
		for(SList<Thread>::iterator it = _ecs.begin(); it != _ecs.end(); ++it, ++i) {
			// it's safe to delete it when either the Thread has re-entered the critical section
			// or if it's out of its critical section. in both cases the Thread would re-read the
			// pointer and thus, can't get the object again we're about to delete.
			uint32_t counter = it->_rcu_counter;
			if(!((counter >> 16) != (_versions[i] >> 16) || (counter & 0xFFFF) == 0))
				return false;
		}
		return true;
	}

	static void store_versions() {
		// new Ecs added?
		if(_versions_count != _ecs.length()) {
			delete[] _versions;
			_versions_count = _ecs.length();
			_versions = new uint32_t[_ecs.length()];
		}

		// update the version-numbers for all Ecs
		ScopedLock<UserSm> guard(&_ecsm);
		size_t i = 0;
		for(SList<Thread>::iterator it = _ecs.begin(); it != _ecs.end(); ++it, ++i)
			_versions[i] = it->_rcu_counter;
	}

	RCU();
	~RCU();
	RCU(const RCU&);
	RCU& operator=(const RCU&);

	static uint32_t *_versions;
	static size_t _versions_count;
	static SList<Thread> _ecs;
	static RCUObject *_objs;
	static UserSm _sm;
	// note that we use a separate lock for the ec list, because it might happen that someone
	// destroys a thread with the destructor of an RCUObject. so, when we used the same Sm,
	// it would cause a deadlock.
	static UserSm _ecsm;
	static RCULock _lock;
};

}
