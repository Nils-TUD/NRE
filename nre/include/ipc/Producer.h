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

#include <mem/DataSpace.h>
#include <ipc/Consumer.h>
#include <util/Sync.h>
#include <util/Math.h>
#include <cstdlib>

#include <stream/Serial.h>

namespace nre {

/**
 * Producer-part for the producer-consumer-communication over a dataspace.
 */
template<typename T>
class Producer {
public:
	/**
	 * Creates a producer that uses the given dataspace for communication
	 *
	 * @param ds the dataspace
	 * @param init whether the producer should init the state. this should only be done by one
	 * 	party and preferably by the first one. That is, if the client is the producer it should
	 * 	init it (because it will create the dataspace and share it to the service).
	 */
	explicit Producer(DataSpace *ds,bool init = true)
		: _ds(ds), _if(reinterpret_cast<typename Consumer<T>::Interface*>(ds->virt())),
		  _max(Math::prev_pow2((ds->size() - sizeof(typename Consumer<T>::Interface)) / sizeof(T))),
		  _sm(_ds->sel(),true) {
		if(init) {
			_if->rpos = 0;
			_if->wpos = 0;
		}
	}

	/**
	 * @return the length of the ring-buffer
	 */
	size_t rblength() const {
		return _max;
	}

	/**
	 * If the client is currently not able to accept it, the method will return 0.
	 *
	 * @return the pointer to the slot to write to or 0 if there is no free slot at the moment
	 */
	T *current() {
		// is it full?
		if(EXPECT_FALSE(((_if->wpos + 1) & (_max - 1)) == _if->rpos))
			return 0;
		return _if->buffer + _if->wpos;
	}

	/**
	 * Moves to the next slot. That is, the position is moved forward and the consumer is notified,
	 * that new data is available
	 */
	void next() {
		_if->wpos = (_if->wpos + 1) & (_max - 1);
		Sync::memory_barrier();
		try {
			_sm.up();
		}
		catch(...) {
			// if the client closed the session, we might get here. so, just ignore it.
		}
	}

	/**
	 * Produces the given item. This is a convenience method which waits for a free slot, copies
	 * the given item into it and moves to the next.
	 *
	 * @param value the value to produce
	 * @return true if the item has been written successfully
	 */
	bool produce(const T &value) {
		T *slot = current();
		if(slot) {
			*slot = value;
			next();
		}
		return slot != 0;
	}

private:
	DataSpace *_ds;
	typename Consumer<T>::Interface *_if;
	size_t _max;
	Sm _sm;
};

}
