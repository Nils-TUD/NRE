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

template<typename T>
class Consumer {
	friend class Producer<T>;

	struct Interface {
		volatile size_t rpos;
		volatile size_t wpos;
		T buffer[];
	};

public:
	Consumer(DataSpace *ds,bool init = false)
		: _ds(ds), _if(reinterpret_cast<Interface*>(ds->virt())),
		  _max((ds->size() - sizeof(Interface)) / sizeof(T)),
		  _sm(_ds->sel(),true), _empty(_ds->unmapsel(),true), _stop(false) {
		if(init) {
			_if->rpos = 0;
			_if->wpos = 0;
		}
	}

	void stop() {
		_stop = true;
		Sync::memory_barrier();
		_sm.up();
	}

	T *get() {
		while(_if->rpos == _if->wpos) {
			if(_stop)
				return 0;
			// sm.down() might fail if the service is the consumer
			try {
				_sm.down();
			}
			catch(...) {
				return 0;
			}
		}
		return _if->buffer + _if->rpos;
	}
	void next() {
		_if->rpos = (_if->rpos + 1) % _max;
		if(_if->rpos == _if->wpos)
			_empty.up();
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
