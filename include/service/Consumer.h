/*
 * (c) 2012 Nils Asmussen <nils@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 *
 * This file is part of TUD:OS and distributed under the terms of the
 * GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#pragma once

#include <mem/DataSpace.h>

namespace nul {

template<typename T>
class Producer;

/**
 * Consumer-part for the producer-consumer-communication over a dataspace.
 *
 * Usage-example:
 * Consumer<char> cons(&ds);
 * for(char *c; (c = cons->get()) != 0; cons.next()) {
 *   // do something with *c
 * }
 */
template<typename T>
class Consumer {
	friend class Producer<T>;

	struct Interface {
		volatile size_t rpos;
		volatile size_t wpos;
		T buffer[];
	};

public:
	/**
	 * Creates a consumer that uses the given dataspace for communication
	 *
	 * @param ds the dataspace
	 * @param init whether the consumer should init the state. this should only be done by one
	 * 	party and preferably by the first one. That is, if the client is the consumer it should
	 * 	init it (because it will create the dataspace and share it to the service).
	 */
	explicit Consumer(DataSpace *ds,bool init = false)
		: _ds(ds), _if(reinterpret_cast<Interface*>(ds->virt())),
		  _max((ds->size() - sizeof(Interface)) / sizeof(T)),
		  _sm(_ds->sel(),true), _empty(_ds->unmapsel(),true), _stop(false) {
		if(init) {
			_if->rpos = 0;
			_if->wpos = 0;
		}
	}

	/**
	 * Stops waiting for the producer. This way, if get() is blocked on the semaphore, it will
	 * be unblocked.
	 */
	void stop() {
		_stop = true;
		Sync::memory_barrier();
		_sm.up();
	}

	/**
	 * @return whether there is more data to read
	 */
	bool has_data() const {
		return _if->rpos != _if->wpos;
	}

	/**
	 * Retrieves the item at current position. If there is no item anymore, it blocks until the
	 * producer notifies it, that there is data available. You might interrupt that by using stop().
	 * Note that the method will only return 0 if it has been stopped *and* there is no data anymore.
	 *
	 * Important: You have to call next() to move to the next item.
	 *
	 * @return pointer to the data
	 */
	T *get() {
		while(_if->rpos == _if->wpos) {
			if(_stop)
				return 0;
			// sm.zero() might fail if the service is the consumer
			try {
				_sm.zero();
			}
			catch(...) {
				return 0;
			}
		}
		return _if->buffer + _if->rpos;
	}

	/**
	 * Tells the producer that you're done working with the current item (i.e. the producer will
	 * never touch the item while you're working with it)
	 */
	void next() {
		_if->rpos = (_if->rpos + 1) % _max;
		if(_if->rpos == _if->wpos) {
			Sync::memory_barrier();
			_empty.up();
		}
	}

private:
	DataSpace *_ds;
	Interface *_if;
	size_t _max;
	Sm _sm;
	Sm _empty;
	bool _stop;
};

}
