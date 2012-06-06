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
	Consumer(DataSpace *ds)
		: _ds(ds), _if(reinterpret_cast<Interface*>(ds->virt())),
		  _max((ds->size() - sizeof(Interface)) / sizeof(T)), _sm(_ds->sel(),true) {
	}

	T *get() {
		if(_if->rpos == _if->wpos)
			_sm.zero();
		return _if->buffer + _if->rpos;
	}
	void next() {
		_if->rpos = (_if->rpos + 1) % _max;
	}

private:
	DataSpace *_ds;
	Interface *_if;
	size_t _max;
	Sm _sm;
};

}
