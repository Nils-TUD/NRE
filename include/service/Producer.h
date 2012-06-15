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
#include <Sync.h>

namespace nul {

template<typename T>
class Producer {
public:
	Producer(DataSpace *ds,bool init = true)
		: _ds(ds), _if(reinterpret_cast<typename Consumer<T>::Interface*>(ds->virt())),
		  _max((ds->size() - sizeof(typename Consumer<T>::Interface)) / sizeof(T)),
		  _sm(_ds->sel(),true), _empty(_ds->unmapsel(),true) {
		if(init) {
			_if->rpos = 0;
			_if->wpos = 0;
		}
	}

	void produce(T &value) {
		// wait until its not empty
		while((_if->wpos + 1) % _max == _if->rpos)
			_empty.down();

		size_t old = _if->wpos;
		_if->buffer[_if->wpos] = value;
		_if->wpos = (_if->wpos + 1) % _max;
		Sync::memory_barrier();
		if(old == _if->rpos) {
			try {
				_sm.up();
			}
			catch(...) {
				// if the client closed the session, we might get here. so, just ignore it.
			}
		}
	}

private:
	DataSpace *_ds;
	typename Consumer<T>::Interface *_if;
	size_t _max;
	Sm _sm;
	Sm _empty;
};

}
