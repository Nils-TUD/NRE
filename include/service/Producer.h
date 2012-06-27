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
#include <service/Consumer.h>
#include <util/Sync.h>
#include <util/Math.h>
#include <cstdlib>

namespace nul {

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
	 * @param block whether you want to block if the client can't accept more data
	 * @param init whether the producer should init the state. this should only be done by one
	 * 	party and preferably by the first one. That is, if the client is the producer it should
	 * 	init it (because it will create the dataspace and share it to the service).
	 */
	explicit Producer(DataSpace *ds,bool block = true,bool init = true)
		: _ds(ds), _if(reinterpret_cast<typename Consumer<T>::Interface*>(ds->virt())),
		  _max(Math::prev_pow2((ds->size() - sizeof(typename Consumer<T>::Interface)) / sizeof(T))),
		  _sm(_ds->sel(),true), _empty(_ds->unmapsel(),true), _block(block) {
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
	 * Produces the given item. If the client is currently not able to accept it, the method will
	 * either block (if enabled) or return false.
	 *
	 * @param value the value to produce
	 * @return true if the item has been written successfully (always true if blocking is enabled)
	 */
	bool produce(const T &value) {
		// wait until its not full anymore
		while(EXPECT_FALSE(((_if->wpos + 1) & (_max - 1)) == _if->rpos)) {
			if(!_block)
				return false;
			_sm.up();
			_empty.zero();
		}

		bool was_empty = _if->wpos == _if->rpos;
		Sync::memory_barrier();
		_if->buffer[_if->wpos] = value;
		_if->wpos = (_if->wpos + 1) & (_max - 1);
		Sync::memory_barrier();
		if(EXPECT_FALSE(was_empty)) {
			try {
				_sm.up();
			}
			catch(...) {
				// if the client closed the session, we might get here. so, just ignore it.
			}
		}
		return true;
	}

private:
	DataSpace *_ds;
	typename Consumer<T>::Interface *_if;
	size_t _max;
	Sm _sm;
	Sm _empty;
	bool _block;
};

}
